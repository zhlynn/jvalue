#include "json.h"

int main(int argc, char *argv[])
{
    jvalue jv;
    jv["happy"] = true;
    jv["name"] = "Niels";
    jv["answer"]["everything"] = 66;

    printf("%s\n", jv.style_write().c_str());
    printf("%s\n", jv.style_write().c_str());

    return 0;
}
