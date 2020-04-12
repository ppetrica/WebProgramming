cl /MD /O2 /I. /I./zlib /I./jansson server_web.c requests.c hash_map/hash_map.c /link /libpath:jansson /libpath:zlib Ws2_32.lib zlibwapi.lib jansson.lib
