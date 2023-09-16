#include <stdio.h>
#include <string.h>

#include "wasm3.h"
#include "m3_env.h"

//#include "extra/fib32.wasm.h"
#include "extra/coremark_minimal.wasm.h"
#include "cm.h"
#include "llapi.h"
#include "wasi.h"

m3ApiRawFunction(env_clock_ms)
{
    m3ApiReturnType (uint32_t)
    m3ApiReturn(llapi_get_tick_ms());
}

/*
void m3_fib()
{
    M3Result result = m3Err_none;

    uint8_t* wasm = (uint8_t*)fib32_wasm;
    size_t fsize = fib32_wasm_len;

    IM3Environment env = m3_NewEnvironment ();
    IM3Runtime runtime = m3_NewRuntime (env, 1024, NULL);
    IM3Module module;
    result = m3_ParseModule (env, &module, wasm, fsize);
    result = m3_LoadModule (runtime, module);

    IM3Function f;
    result = m3_FindFunction (&f, runtime, "fib");

    printf("wasm run:%d\r\n", llapi_get_tick_ms());
    result = m3_CallV(f, 24);
    uint32_t value = 0;
    result = m3_GetResultsV (f, &value);
    printf("wasm fin:%d, %d\r\n", llapi_get_tick_ms(), value);
}*/

IM3Environment  env;
IM3Runtime      runtime;
IM3Module       module;

IM3Function     func_run;
#define FATAL(func, msg) { printf("Fatal: " func " "); printf("%s \r\n",msg); while(1) { llapi_delay_ms(100); } }

int main()
{

    M3Result result = m3Err_none;

    env = m3_NewEnvironment ();
    if (!env) FATAL("NewEnvironment", "failed");

    runtime = m3_NewRuntime (env, 32*1024, NULL);
    if (!runtime) FATAL("NewRuntime", "failed");

    runtime->memoryLimit = 128*1024;

    //result = m3_ParseModule (env, &module, coremark_minimal_wasm, coremark_minimal_wasm_len);
    result = m3_ParseModule (env, &module, data, sizeof(data));
    if (result) FATAL("ParseModule", result);

    result = m3_LoadModule (runtime, module);
    if (result) FATAL("LoadModule", result);

    result = m3_LinkEspWASI (runtime->modules);
    if (result) FATAL("m3_LinkEspWASI: %s", result);
    m3_LinkRawFunction (module, "env",    "clock_ms",   "i()",      &env_clock_ms);
    
    result = m3_FindFunction (&func_run, runtime, "_start");
    //result = m3_FindFunction (&func_run, runtime, "run");

    if (result) FATAL("FindFunction", result);

    printf("Running CoreMark 1.0...\r\n");


    const char* i_argv[2] = { "test.wasm", NULL };

    m3_wasi_context_t* wasi_ctx = m3_GetWasiContext();
    wasi_ctx->argc = 1;
    wasi_ctx->argv = i_argv;

    result = m3_CallV (func_run);

    if (result) FATAL("m3_Call: ", result);
    //result = m3_CallV (func_run);
    //if (result) FATAL("Call", result);

    float value = 0;
    result = m3_GetResultsV (func_run, &value);
    if (result) FATAL("GetResults: ", result);

    if (result != m3Err_none) {
        M3ErrorInfo info;
        m3_GetErrorInfo (runtime, &info);
        printf("Error: ");
        printf("%s",result);
        printf(" (");
        printf("%s",info.message);
        printf(")\r\n");
        if (info.file && strlen(info.file) && info.line) {
            printf("At ");
            printf("%s",info.file);
            printf(":");
            printf("%d\r\n",info.line);
        }
    }

    while (1)
    {
        /* code */
    }

}

