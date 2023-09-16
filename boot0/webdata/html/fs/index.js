import { getErrorMessage } from "./lfs_err.js";
import {
    LFS, BlockDevice, MemoryBlockDevice, LFSModule,
    LFS_O_CREAT, LFS_O_RDONLY, LFS_O_WRONLY, LFS_O_TRUNC,
    LFS_TYPE_DIR, LFS_TYPE_REG,
    LFS_ERR_OK,
    LFS_ERR_CORRUPT,
} from "./lfs_js.js";

const sleep = (ms) => new Promise(res => setTimeout(res, ms));

/**
 * download file to local
 * @param {Uint8Array} data 
 * @param {string} name 
 */
export function saveLocalFile(data, name) {
    var blob = new Blob([data.buffer], {
        type: "application/octet-stream"
    });
    let a = document.createElement("a");
    a.download = name;
    a.href = URL.createObjectURL(blob);
    a.click();
}

/**
 * open a local file. may return undefined
 * @param {string} accept
 * @param {number} [timeout=30000]
 * @returns {Promise<Array<File>|undefined>}
 */
export function openLocalFile(accept = "*/*", timeout = 300000) {
    let ip = document.createElement("input");
    ip.type = "file";
    ip.accept = accept;
    return new Promise((resolve) => {
        let timerId = setTimeout(() => {
            ip.onchange = undefined;
            resolve(undefined);
        }, timeout);
        ip.onchange = (event) => {
            clearTimeout(timerId);
            ip.onchange = undefined;
            let files = ip.files;
            if (files.length <= 0) {
                resolve(undefined);
                return;
            }
            resolve(files);
        };
        ip.click();
    });
}

/**
 * Read a file object
 * @param {File} file
 * @returns {Promise<Uint8Array>} 
 */
const readFile = (file) => {
    return new Promise((res, rej) => {
        const reader = new FileReader();
        reader.onerror = rej;
        reader.onabort = rej;
        reader.onloadend = (ev) => {
            /** @type {ArrayBuffer} */
            const buffer = reader.result;
            res(new Uint8Array(buffer));
        };
        reader.readAsArrayBuffer(file);
    });
};

class HttpBlockDevice extends BlockDevice {
    constructor(uri) {
        super();
        this.read_size = 2048;
        this.prog_size = 2048;
        this.block_size = (2048) * 64;
        this.block_count = 1024 - 48;
        this.uri = uri; 
    }
    async read(block, off, buffer, size) {
        await fetch(this.uri + '/nandread_block=' + block + '_off=' + off + '_size=' + size).then(
            response => {
                return response.arrayBuffer()
            }).then(res => {
                //console.log(res);
                if(res.byteLength < 10)
                {
                    var s = String.fromCharCode(null, new Uint8Array(res));
                    console.log(s);
                    return LFS_ERR_CORRUPT;
                }
                LFSModule.HEAPU8.set(
                new Uint8Array(res),
                buffer);
            })
        return 0;
    }
    async prog(block, off, buffer, size) {
        var dat = new Uint8Array(LFSModule.HEAPU8.buffer, buffer, size);

        await fetch(this.uri + '/nandwrite_block=' + block + '_off=' + off + '_size=' + size, 
            {
                method: 'POST', 
                body: dat
            }).then(function(data){
                //console.log(data);
            })
        return 0;
    }
    async erase(block) {
        await fetch(this.uri + '/nanderase_block=' + block).then(
            response => {
                return response.arrayBuffer()
            }).then(res => {
                //console.log(res);
            })
        return 0;
    }
}

let cwd = "/";
let bdev = new MemoryBlockDevice(128, 1024);
let lfs = new LFS(bdev, 100);
let file_upload_download_size = 256;

const eDiv = (...children) => {
    const elem = document.createElement("div");
    for (const child of children) {
        elem.append(child);
    }
    return elem;
};

const eTd = (...children) => {
    const elem = document.createElement("td");
    for (const child of children) {
        elem.append(child);
    }
    return elem;
};

const eButton = (text, data, onClick) => {
    const elem = document.createElement("button");
    elem.innerText = text;
    elem.dataset["data"] = data;
    elem.addEventListener("click", onClick);
    return elem;
};

const pathJoin = (base, ...parts) => {
    if (base.endsWith("/")) {
        base = base.substring(0, base.length - 1);
    }
    /** @type {Array<string>} */
    const partList = base.split("/");
    for (const part of parts) {
        if (typeof (part) !== "string" || part.length <= 0 || part === ".") {
            continue;
        } else if (part === "..") {
            partList.pop();
        } else {
            partList.push(part);
        }
    }
    if (partList.length <= 1) {
        return "/";
    } else {
        return partList.join("/");
    }
};

const renderModePrompt = async (msg, value, onCancel, onConfirm) => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv(msg));
    const field = document.createElement("input");
    field.type = "text";
    field.value = value;
    container.append(field);
    container.append(eDiv(
        eButton("Cancel", null, () => {
            onCancel();
        }),
        eButton("Confirm", null, () => {
            const inputValue = field.value;
            onConfirm(inputValue);
        }),
    ));
};

// init user interface
const renderModeInit = async () => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv("URI:"));
    const text1 = document.createElement("input");
    const btn1 = document.createElement("button");
    text1.value = "http://192.168.77.1";
    btn1.innerText = "Connect";
    btn1.addEventListener("click", async () => {
        console.log(text1.value);
        bdev = new HttpBlockDevice(text1.value);
        lfs = new LFS(bdev, 1000);
        container.append(eDiv("Mounting filesystem..."));
        btn1.disabled = true;
        var res = await lfs.mount();
        container.append(eDiv(`mount: ${ res }`));
        btn1.disabled = false;
        if(res != LFS_ERR_OK)
        {
            container.append(eDiv("FS Not Found."));
        }else{
            await renderModeList();
        }
    }); 
    container.append(text1);
    container.append(btn1);
};

// list files
const renderModeList = async () => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    const labelCWD = document.createElement("h6");
    labelCWD.innerText = `CWD: ${cwd}`;
    container.append(labelCWD);

    
    const dir = await lfs.opendir(cwd);
    
    // toolbar
    container.append(eDiv(
        eButton("Refresh", cwd, renderModeList),
        eButton("Up Dir", cwd, async () => {
            const newCWD = pathJoin(cwd, "..");
            cwd = newCWD;
            await renderModeList();
        }),
        eButton("Create Dir", cwd, async () => {
            await renderModePrompt(
                "Dir path:", cwd, renderModeList,
                async (path) => {
                    await renderModeCreateDir(path);
                },
            );
        }),
        eButton("Upload File", cwd, async () => {
            const files = await openLocalFile();
            for (const file of files) {
                await renderModeUpload(file, cwd);
            }
            await renderModeList();
        }),
    ));

    // read dir
    container.append(document.createElement("hr"));
    if (typeof (dir) === "number") {
        console.error("opendir:", cwd, dir);
        return;
    }
    const table = document.createElement("table");
    container.append(table);
    const tableHeader = document.createElement("thead");
    table.append(tableHeader);
    const tableHeaderRow = document.createElement("tr");
    tableHeader.append(tableHeaderRow);
    for (const headerName of ["Filename", "Type", "Size", "Operation"]) {
        const header = document.createElement("th");
        header.innerText = headerName;
        tableHeaderRow.append(header);
    }
    const tableBody = document.createElement("tbody");
    table.append(tableBody);

    while (true) {
        const ent = await dir.read();
        if (ent === null) {
            break;
        }
        const filename = ent.name;
        const item = document.createElement("tr");
        if (ent.type === LFS_TYPE_DIR) {
            item.append(eTd(filename));
            item.append(eTd("Dir"));
            item.append(eTd("--"));
            if (filename === "." || filename === "..") {
                continue;
            }
            item.append(eTd(
                eButton("Open", filename, async (e) => {
                    const newCWD = pathJoin(cwd, e.target.dataset["data"]);
                    cwd = newCWD;
                    await renderModeList();
                }),
                eButton("Move (Rename)", filename, async (e) => {
                    const pathFrom = pathJoin(cwd, e.target.dataset["data"]);
                    await renderModePrompt(
                        "New path:", pathFrom, renderModeList,
                        async (pathTo) => {
                            await renderModeMove(pathFrom, pathTo);
                        },
                    );
                }),
                eButton("Delete", filename, async (e) => {
                    await renderModeDelete(pathJoin(cwd, e.target.dataset["data"]));
                }),
            ));
        } else if (ent.type === LFS_TYPE_REG) {
            item.append(eTd(filename));
            item.append(eTd("File"));
            item.append(eTd(ent.size.toString()));
            item.append(eTd(
                eButton("Download", filename, async (e) => {
                    await renderModeDownload(cwd, e.target.dataset["data"]);
                    await renderModeList();
                }),
                eButton("Move (Rename)", filename, async (e) => {
                    const pathFrom = pathJoin(cwd, e.target.dataset["data"]);
                    await renderModePrompt(
                        "New path:", pathFrom, renderModeList,
                        async (pathTo) => {
                            await renderModeMove(pathFrom, pathTo);
                        },
                    );
                }),
                eButton("Delete", filename, async (e) => {
                    await renderModeDelete(pathJoin(cwd, e.target.dataset["data"]));
                }),
            ));
        } else {
            continue;
        }
        tableBody.append(item);
    }
};

/**
 * Upload a file
 * @param {File} file 
 * @param {string} cwd 
 */
const renderModeUpload = async (file, cwd) => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv("Uploading..."));
    const newFilePath = pathJoin(cwd, file.name);
    const content = await readFile(file);
    const fileObj = await lfs.open(newFilePath, LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC);
    if (typeof (fileObj) === "number") {
        container.append(eDiv(`Error: ${getErrorMessage(resultCode)}`));
        container.append(eButton("Back", null, renderModeList));
        return;
    }
    container.append(eDiv(`open: ${newFilePath}`));
    let offset = 0;
    while (offset < content.length) {
        const msg = eDiv(`write: ${offset}/${content.length}`);
        container.append(msg);
        const blockSize = Math.min(file_upload_download_size, content.length - offset);
        const writeSize = await fileObj.write(content.slice(offset, offset + blockSize));
        container.removeChild(msg);
        if (writeSize < 0) {
            container.append(eDiv(`Error: ${getErrorMessage(writeSize)}`));
            container.append(eButton("Back", null, renderModeList));
            throw new Error(getErrorMessage(writeSize));
        }
        offset += writeSize;
    }
    let ret = await fileObj.sync();
    if (ret < 0) {
        container.append(eDiv(`Error: ${getErrorMessage(ret)}`));
        container.append(eButton("Back", null, renderModeList));
        throw new Error(getErrorMessage(ret));
    }
    ret = await fileObj.close();
    if (ret < 0) {
        container.append(eDiv(`Error: ${getErrorMessage(ret)}`));
        container.append(eButton("Back", null, renderModeList));
        throw new Error(getErrorMessage(ret));
    }
};

const renderModeDownload = async (cwd, name) => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv("Downloading..."));
    const filePath = pathJoin(cwd, name);
    const fileObj = await lfs.open(filePath, LFS_O_RDONLY);
    if (typeof (fileObj) === "number") {
        container.append(eDiv(`Error: ${getErrorMessage(resultCode)}`));
        container.append(eButton("Back", null, renderModeList));
        return;
    }
    container.append(eDiv(`open: ${filePath}`));
    const contentSize = await fileObj.size();
    const buffer = new Uint8Array(contentSize);
    let offset = 0;
    while (offset < contentSize) {
        const msg = eDiv(`read: ${offset}/${contentSize}`);
        container.append(msg);
        const blockSize = Math.min(file_upload_download_size, contentSize - offset);
        const data = await fileObj.read(blockSize);
        container.removeChild(msg);
        if (typeof (data) === "number") {
            container.append(eDiv(`Error: ${getErrorMessage(data)}`));
            container.append(eButton("Back", null, renderModeList));
            throw new Error(getErrorMessage(data));
        }
        buffer.set(data, offset);
        offset += data.length;
    }
    saveLocalFile(buffer, name);
};

const renderModeCreateDir = async (path) => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv("Creating dir..."));
    const resultCode = await lfs.mkdir(path);
    if (resultCode !== LFS_ERR_OK) {
        container.append(eDiv(`Error: ${getErrorMessage(resultCode)}`));
        container.append(eButton("Back", null, renderModeList));
    } else {
        await renderModeList();
    }
};

const renderModeDelete = async (path) => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv("Deleting..."));
    const resultCode = await lfs.remove(path);
    if (resultCode !== LFS_ERR_OK) {
        container.append(eDiv(`Error: ${getErrorMessage(resultCode)}`));
        container.append(eButton("Back", null, renderModeList));
    } else {
        await renderModeList();
    }
};

const renderModeMove = async (pathFrom, pathTo) => {
    const container = document.querySelector("#container");
    container.innerHTML = "";
    container.append(eDiv("Moving..."));
    const resultCode = await lfs.rename(pathFrom, pathTo);
    if (resultCode !== LFS_ERR_OK) {
        container.append(eDiv(`Error: ${getErrorMessage(resultCode)}`));
        container.append(eButton("Back", null, renderModeList));
    } else {
        await renderModeList();
    }
};

window.addEventListener("load", async () => {
    await renderModeInit();
});