import {
    LFS_ERR_OK,
    LFS_ERR_IO,
    LFS_ERR_CORRUPT,
    LFS_ERR_NOENT,
    LFS_ERR_EXIST,
    LFS_ERR_NOTDIR,
    LFS_ERR_ISDIR,
    LFS_ERR_NOTEMPTY,
    LFS_ERR_BADF,
    LFS_ERR_FBIG,
    LFS_ERR_INVAL,
    LFS_ERR_NOSPC,
    LFS_ERR_NOMEM,
    LFS_ERR_NOATTR,
    LFS_ERR_NAMETOOLONG,
} from "./lfs_js.js";

const ERR_MAP = {
    [LFS_ERR_OK]: "No error",
    [LFS_ERR_IO]: "Error during device operation",
    [LFS_ERR_CORRUPT]: "Corrupted",
    [LFS_ERR_NOENT]: "No directory entry",
    [LFS_ERR_EXIST]: "Entry already exists",
    [LFS_ERR_NOTDIR]: "Entry is not a dir",
    [LFS_ERR_ISDIR]: "Entry is a dir",
    [LFS_ERR_NOTEMPTY]: "Dir is not empty",
    [LFS_ERR_BADF]: "Bad file number",
    [LFS_ERR_FBIG]: "File too large",
    [LFS_ERR_INVAL]: "Invalid parameter",
    [LFS_ERR_NOSPC]: "No space left on device",
    [LFS_ERR_NOMEM]: "No more memory available",
    [LFS_ERR_NOATTR]: "No data/attr available",
    [LFS_ERR_NAMETOOLONG]: "File name too long",
};

export const getErrorMessage = (code) => {
    return ERR_MAP[code];
};
