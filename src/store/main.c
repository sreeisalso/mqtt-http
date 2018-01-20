#include <stdio.h>
#include <signal.h>
#include "store.h"
int stop;
static void handle_signal(int s)
{
    stop = 1;
}

int main(int argc, char *argv[])
{
    JSON_Value *file = NULL;
    JSON_Object *jconfig = NULL;
    config_t config;
    int rc = 0;

    if (argc < 2)
    {
        printf("Run as %s <config file>\n", argv[0]);
        return -1;
    }

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    file = json_parse_file(argv[1]);
    if (JSONObject != json_value_get_type(file))
    {
        printf("config file must strat as Json object\n");
        return -2;
    }

    jconfig = json_value_get_object(file);

    rc = mongodb_init(jconfig, &config);
    if (rc)
    {
        printf("Invalid mongodb config, rc = %d\n", rc);
        return -3;
    }

    rc = mosquitto_init(jconfig, &config);
    if (rc)
    {
        printf("Invalid mosquitto config, rc = %d\n", rc);
        return -5;
    }

    while (!stop)
    {
        //do the actula work here
        rc = mosquitto_loop(config.mosquitto, -1, 1);
        if (rc)
        {
            printf("mosquitto_loop error : %d\n", rc);
            return -6;
        }
    }
    mongodb_destroy(&config);
    mosquitto_destroy(config.mosquitto);
    mosquitto_lib_cleanup();
    return 0;
}