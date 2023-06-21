const express = require('express');
const exec = require('node:child_process').exec;
const fs = require('fs');
const cors = require('cors');
const uuid = require('uuid');
var crypto = require('crypto');

const app = express();
app.use(cors());
const port = 3000;

app.use(express.json({limit: '50mb'}));
app.use(express.urlencoded({limit: '50mb',extended: true}));

const SQLITE_TOOLS_DIR='C:/lulu/sqlite-tools-win32-x86-3410200';
const DEPLOY_DIR='C:/tmp/linkedboxdraw-master';
const TEMP_DIR='C:/tmp';


const corsOptions = {
    origin: 'https://ludoaubert.github.io',
    optionsSuccessStatus: 200, // For legacy browser support
    methods: "GET, PUT"
};

app.use(cors(corsOptions));


app.get('/list_diagrams', (req, res) => {
    exec(`${SQLITE_TOOLS_DIR}/sqlite3 linkedboxdraw.db "SELECT json_group_array(json_object('title',title, 'deleted', deleted, 'guid', guid)) FROM diagram"`,(error, stdout, stderr) => {
        console.log("STDOUT:", stdout, ", STDERR:", stderr);
	    res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
		res.setHeader('Access-Control-Allow-Origin', '*');
        res.end(stdout);
	});
});
	
app.get('/get_document', (req, res) => {
	const guid = req.query.guid;
	const query = fs.readFileSync(`${DEPLOY_DIR}/select_document.sql`, 'utf8')
					.replaceAll('a8828ddfef224d36935a1c66ae86ebb3', guid);
	console.log(query);
	fs.writeFileSync(`${TEMP_DIR}/select_document_${guid}.sql`, query);
    exec(`${SQLITE_TOOLS_DIR}/sqlite3 linkedboxdraw.db ".read ${TEMP_DIR}/select_document_${guid}.sql"`, { maxBuffer: Infinity },(error, stdout, stderr) => {
        console.log("STDOUT:", stdout, ", STDERR:", stderr);
		console.log(guid);
	    res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
		res.setHeader('Access-Control-Allow-Origin', '*');
        res.end(stdout);
	});		
	
	var hash = crypto.createHash('sha512');
//passing the data to be hashed
	const data = hash.update('nodejsera', 'utf-8');
//Creating the hash in the required format
	const gen_hash = data.digest('hex');
//Printing the output on the console
	console.log("hash : " + gen_hash);
});


app.post('/set_document', (req, res) => {
	console.log("POST hit!");
	const guid = uuid.v4();
	const query = fs.readFileSync(`${DEPLOY_DIR}/insert_document.sql`, 'utf8')
					.replaceAll('a8828ddfef224d36935a1c66ae86ebb3', guid)
					.replace('${diagData}', JSON.stringify(req.body.data))
					.replace('${geoData}', JSON.stringify(req.body.contexts))
					.replace('${title}', req.body.data.documentTitle);
	console.log(query);
	fs.writeFileSync(`${TEMP_DIR}/insert_document_${guid}.sql`, query);
    exec(`${SQLITE_TOOLS_DIR}/sqlite3 linkedboxdraw.db ".read ${TEMP_DIR}/insert_document_${guid}.sql"`,(error, stdout, stderr) => {
        console.log("STDOUT:", stdout, ", STDERR:", stderr);
		res.statusCode = 200;
		res.setHeader('Content-Type', 'application/json');
		res.setHeader('Access-Control-Allow-Origin', '*');
		res.send('Data Received: ' + JSON.stringify(req.body));
	});
	
	for (let {height, width, name, base64, zoomPercentage} of req.body.data.pictures)
	{
		var hash = crypto.createHash('sha512');
		const data = hash.update(base64, 'utf-8');
		const gen_hash = data.digest('hex');
		console.log("hash : " + gen_hash);
//		const blob = atob(base64);
		const blob = Buffer.from(base64, 'base64').toString('binary');
		fs.writeFileSync(`${DEPLOY_DIR}/images/${gen_hash}.jpg`, blob);
	}
});


app.listen(port, () => {
  console.log(`Server listening on port ${port}!`);
});