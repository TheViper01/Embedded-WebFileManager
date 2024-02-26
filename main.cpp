#include "mongoose/mongoose.h"

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

// ab -n 100000 -c 100 http://localhost:8081/

// {"items": [{"path":"/shared/Screenshot 2023-03-02 143014.png", "name" : "Screenshot 2023-03-02 143014.png", "size" : 115334, "extension" : ".png", "modified" : "2024-02-17T07:08:05.968Z", "mode" : 493, "isDir" : false, "isSymlink" : false, "type" : "image"}, { "path":"/shared/Test2PJ_tip1.zip","name" : "Test2PJ_tip1.zip","size" : 19936,"extension" : ".zip","modified" : "2023-12-14T05:12:33.876Z","mode" : 493,"isDir" : false,"isSymlink" : false,"type" : "blob" }] , "numDirs" : 0, "numFiles" : 2, "sorting" : {"by":"name", "asc" : false}, "path" : "/shared/", "name" : "shared", "size" : 4096, "extension" : "", "modified" : "2024-02-17T07:08:05.96Z", "mode" : 2147484141, "isDir" : true, "isSymlink" : false, "type" : ""}



const char HTTP_METHOD_GET[] = "GET";       // download file or list directory
const char HTTP_METHOD_POST[] = "POST";     // Create directory
const char HTTP_METHOD_PUT[] = "PUT";       // upload file or replace if exist
const char HTTP_METHOD_DELETE[] = "DELETE"; // delete file
const char HTTP_METHOD_PATCH[] = "PATCH";   // rename file

const char API_FILE_ENDPOINT[] = "/api/files";
const char API_FRONTEND_RESOURCE_ENDPOINT[] = "/resources";
const char FILE_DIRECTORY[] = "web_root/files";
const char FRONTEND_FILE_MANAGER[] = "web_root/frontend/file_manager";
const char FRONTEND_FILE_DIRECTORY[] = "web_root/frontend";
const char FRONTEND_FILE_API_ENDPOINT[] = "/frontend";


static void main_callback(struct mg_connection* c, int ev, void* ev_data, void* fn_data) {
    struct mg_http_message* hm;
    struct mg_http_serve_opts opts = {};
    char tempString[MG_PATH_MAX];
    int flags;
    int temp_len1, temp_len2;

    if (ev == MG_EV_HTTP_MSG) {
        hm = (struct mg_http_message*)ev_data;
        printf("URI: %.*s\n", hm->uri.len, hm->uri.ptr);

        if (hm->uri.len >= strlen(API_FILE_ENDPOINT) && strncasecmp(hm->uri.ptr, API_FILE_ENDPOINT, MIN(hm->uri.len, strlen(API_FILE_ENDPOINT))) == 0) {
            strcpy(tempString, FILE_DIRECTORY);
            memset(tempString + strlen(tempString), 0, MG_PATH_MAX);
            mg_url_decode(hm->uri.ptr + strlen(API_FILE_ENDPOINT), hm->uri.len - strlen(API_FILE_ENDPOINT), &(tempString[strlen(tempString)]), MG_PATH_MAX, 0);
            //memcpy(&(tempString[strlen(tempString)]), hm->uri.ptr + strlen(API_FILE_ENDPOINT), hm->uri.len - strlen(API_FILE_ENDPOINT));
            //printf("Download: %s\n", tempString);
            
            // download file or list directory
            if (mg_vcasecmp(&hm->method, HTTP_METHOD_GET) == 0) {
                printf("Download: %s\n", tempString);
                flags = fileStat(&opts, tempString);
                // list files in directory
                if (flags & MG_FS_DIR) {
                    opts = {};
                    mg_http_dir_list_json(c, hm, &opts, tempString);
                }
                // serve file
                else {
                    opts = {
                        .mime_types = "text/plain"
                    };
                    mg_http_serve_file(c, hm, tempString, &opts);
                }
            }
            // create directory
            else if (mg_vcasecmp(&hm->method, HTTP_METHOD_POST) == 0 && hm->uri.ptr[hm->uri.len - 1] == '/') {
                printf("Create Dir: %s\n", tempString);
                flags = fileStat(&opts, tempString);
                if (flags < 0) {
                    mg_http_reply(c, 400, NULL, "\n");  // bad request
                }
                else if (flags & MG_FS_DIR) {
                    mg_http_reply(c, 409, "conflict", "\n");  // directory already exists
                }
                else {
                    if (mg_mkdir(&opts, tempString))
                    {
                        mg_http_reply(c, 200, NULL, "\n");  // success
                    }
                    else {
                        mg_http_reply(c, 500, NULL, "\n");  // internal error
                    }
                }
            }
            // remove file or directory
            else if (mg_vcasecmp(&hm->method, HTTP_METHOD_DELETE) == 0) {
                printf("Remove: %s\n", tempString);
                flags = fileStat(&opts, tempString);
                if (flags < 0) {
                    mg_http_reply(c, 400, NULL, "\n");  // bad request
                }
                // remove directory
                else if (flags & MG_FS_DIR) {
                    temp_len1 = strlen(FILE_DIRECTORY);
                    temp_len2 = strlen(tempString);
                    if ((temp_len1 == temp_len2) && strncasecmp(tempString, FILE_DIRECTORY, MIN(temp_len1, temp_len2)) == 0) {
                        mg_http_reply(c, 403, NULL, "\n");  // forbidden
                    }
                    else if (((temp_len1+1) == temp_len2) && strncasecmp(tempString, FILE_DIRECTORY, MIN(temp_len1, temp_len2)) == 0 && tempString[temp_len2-1] == '/') {
                        mg_http_reply(c, 403, NULL, "\n");  // forbidden
                    }
                    else if (mg_rmdir(&opts, tempString)) {
                        mg_http_reply(c, 200, NULL, "\n");  // success
                    }
                    else {
                        mg_http_reply(c, 500, tempString, "\n");  // internal error
                    }
                }
                // remove file
                else if (flags & MG_FS_READ || flags & MG_FS_WRITE) {
                    if (mg_rm(&opts, tempString))
                    {
                        mg_http_reply(c, 200, NULL, "\n");  // success
                    }
                    else {
                        mg_http_reply(c, 500, NULL, "\n");  // internal error
                    }
                }
                else {
                    mg_http_reply(c, 404, NULL, "\n");  // directory dies not exists
                }
            }
            // upload file
            else if (mg_vcasecmp(&hm->method, HTTP_METHOD_PUT) == 0) {
                printf("Upload: %s\n", tempString);
                if (tempString[strlen(tempString) - 1] != '/') {
                    mg_http_upload(c, hm, &mg_fs_posix, tempString, UINT_MAX);
                }
                else {
                    mg_http_reply(c, 400, NULL, "\n");  // bad request
                }
                
            }
            // internal error
            else {
                mg_http_reply(c, 500, NULL, "\n");
            }
        }
        // serve files frontend
        else if (hm->uri.len >= strlen(API_FRONTEND_RESOURCE_ENDPOINT) && strncasecmp(hm->uri.ptr, API_FRONTEND_RESOURCE_ENDPOINT, MIN(hm->uri.len, strlen(API_FRONTEND_RESOURCE_ENDPOINT))) == 0) {
            if (mg_vcasecmp(&hm->method, HTTP_METHOD_GET) == 0) {
                if (hm->uri.ptr[hm->uri.len - 1] == '/')
                {
                    strcpy(tempString, FRONTEND_FILE_MANAGER);
                    memset(tempString + strlen(tempString), 0, MG_PATH_MAX);
                    //memcpy(&(tempString[strlen(tempString)]), hm->uri.ptr + strlen(API_FRONTEND_RESOURCE_ENDPOINT), hm->uri.len - strlen(API_FRONTEND_RESOURCE_ENDPOINT));

                    flags = fileStat(&opts, tempString);
                    if (flags & MG_FS_DIR)
                    {
                        if (tempString[strlen(tempString) - 1] != '/') {
                            strcpy(&tempString[strlen(tempString)], "/");
                        }
                        strcpy(&tempString[strlen(tempString)], "index.html");
                    }
                    else {
                        mg_http_reply(c, 400, NULL, "\n");  // bad request
                    }
                }
                else {
                    strcpy(tempString, FILE_DIRECTORY);
                    memset(tempString + strlen(tempString), 0, MG_PATH_MAX);
                    memcpy(&(tempString[strlen(tempString)]), hm->uri.ptr + strlen(API_FILE_ENDPOINT), hm->uri.len - strlen(API_FILE_ENDPOINT));
                }

                printf("file: %s\n", tempString);

                flags = fileStat(&opts, tempString);
                if (flags & MG_FS_DIR)
                {
                    mg_http_reply(c, 400, NULL, "\n");  // bad request
                }
                else {
                    mg_http_serve_file(c, hm, tempString, &opts);
                }
            }
            else {
                mg_http_reply(c, 403, NULL, "\n");  // forbidden
            }
        }
        // frontend file serve
        else if (hm->uri.len >= strlen(FRONTEND_FILE_API_ENDPOINT) && strncasecmp(hm->uri.ptr, FRONTEND_FILE_API_ENDPOINT, MIN(hm->uri.len, strlen(FRONTEND_FILE_API_ENDPOINT))) == 0) {
            if (mg_vcasecmp(&hm->method, HTTP_METHOD_GET) == 0) {
                strcpy(tempString, FRONTEND_FILE_DIRECTORY);
                memset(tempString + strlen(tempString), 0, MG_PATH_MAX);
                memcpy(&(tempString[strlen(tempString)]), hm->uri.ptr + strlen(FRONTEND_FILE_API_ENDPOINT), hm->uri.len - strlen(FRONTEND_FILE_API_ENDPOINT));

                flags = fileStat(&opts, tempString);
                if (flags & MG_FS_DIR) {
                    mg_http_reply(c, 404, NULL, "\n");  // file not found
                }
                else {
                    mg_http_serve_file(c, hm, tempString, &opts);
                }
            }
            else{
                mg_http_reply(c, 403, NULL, "\n");  // forbidden
            }
        }
        else {
            //opts = { .root_dir = "web_root/" };
            //mg_http_serve_dir(c, hm, &opts);
            //mg_http_serve_dir_json(c, hm, &opts);
            mg_http_reply(c, 501, NULL, "\n");
        }
    }
}





int main(void) {
    struct mg_mgr mgr;  // Declare event manager
    mg_mgr_init(&mgr);  // Initialise event manager
    mg_http_listen(&mgr, "http://0.0.0.0:8081", main_callback, NULL);  // Setup listener
    for (;;) {
        mg_mgr_poll(&mgr, 1000);  // Run an infinite event loop
    }
    return 0;
}

