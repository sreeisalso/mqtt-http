PWD := `pwd`
SRC := ${PWD}/src
CFLAGS := -Ilib `pkg-config  --cflags libmongoc-1.0`\
	-I${PWD}/inc
LFLAGS := -lmongoc-1.0 -lbson-1.0 `pkg-config  --libs libmongoc-1.0`\
	 -lmosquitto
CC := gcc

MQTT_STORE_SRC := src/store/main.c\
	src/store/mongodb.c\
	src/store/mosquitto.c\
	lib/parson/parson.c
MQTT_SEND_SRC := src/send/main.c\
	lib/parson/parson.c

all: mqtt_store mqtt_send

mqtt_send: ${MQTT_SEND_SRC}
	${CC} ${CFLAGS} ${MQTT_SEND_SRC} -o mqtt_send ${LFLAGS}

mqtt_store: ${MQTT_STORE_SRC}
	${CC} ${CFLAGS} ${MQTT_STORE_SRC} -o mqtt_store ${LFLAGS}

clean:
	rm -rf mqtt_send mqtt_store

