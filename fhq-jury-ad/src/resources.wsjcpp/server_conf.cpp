// automaticly generated by update_resources.py

#include <server_conf.h>

REGISTRY_WSJCPP_RESOURCE_FILE(RES_server_conf)

const std::string &RES_server_conf::getFilename() {
    static const std::string s = "server.conf";
    return s;
}

const int RES_server_conf::getBufferSize() {
    return 240;
}

const char *RES_server_conf::getBuffer() {
    static const std::string sRet =  // size: 240
        "# use storage which storage will be used, now possible values:\n"
        "# mysql - will be used mysql database\n"
        "# ram - (not implemented yet) use just memory\n"
        "# postgres - (not implemented yet) will be used postgre database\n"
        "\n"
        "server.use_storage = mysql\n"
    ;
    return sRet.c_str();
} //::buffer() 
