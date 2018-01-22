const Express = require('express');
const BodyParser = require('body-parser');
const MongoClient = require('mongodb').MongoClient;
const Debug = require('debug')('mqtt-http-send');
let db;
let topicsdb;
const db_name = 'mqttdb';
const coll_name = 'topicsdb';
const uri_str = "mongodb://localhost:27017/mydb";
const port = 3000;
MongoClient.connect(uri_str)
    .then(con => {
        Debug("Connected to db");
        db = con.db(db_name);
        topicsdb = db.collection(coll_name);
    }).catch(err => {
        Debug("Connected to db failed");
        Debug(err);
    });

const findTopic = (_id) => {
    Debug({ _id });
    return topicsdb.findOne({ _id });
};
const app = Express();
app.use(BodyParser.urlencoded({
    extended: true
}));

app.get('/', (req, res) => {
    //Debug(req);
    const topic = req.query.topic;
    Debug({ topic });
    if (topic) {
        findTopic(topic)
            .then(obj => {
                const msg = obj.msg;
                res.status(200).send({ topic, msg });
            }).catch(err => {
                res.status(400).send(err);
            });
    } else {
        res.status(400).send('Topic not found');
    }
});
app.listen(port, () => Debug(`Server started at http://localhost:${port}`));