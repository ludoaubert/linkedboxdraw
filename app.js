const express = require('express');
const exec = require('node:child_process').exec;
const fs = require('fs');
const cors = require('cors');
const uuid = require('uuid');

const app = express();
app.use(cors());
const port = 3000;

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

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
	const query = fs.readFileSync(`${DEPLOY_DIR}/select_document.sql`, 'utf8')
					.replace(/\s+/g, ' ');
	console.log(query);
	console.log(req.query.guid);
    exec(`${SQLITE_TOOLS_DIR}/sqlite3 linkedboxdraw.db "${query}"`,(error, stdout, stderr) => {
        console.log("STDOUT:", stdout, ", STDERR:", stderr);
		console.log(req.query.guid);
	    res.statusCode = 200;
        res.setHeader('Content-Type', 'application/json');
		res.setHeader('Access-Control-Allow-Origin', '*');
        res.end(stdout);
	});		
});


app.post('/set_document', (req, res) => {
	console.log("POST hit!");
	const guid = uuid.v4();
	const query = fs.readFileSync(`${DEPLOY_DIR}/insert_document.sql`, 'utf8')
					//.replace(/\s+/g, ' ')
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
		res.send('Data Received: ' + JSON.stringify(data));
	});
});


app.listen(port, () => {
  console.log(`Server listening on port ${port}!`);
});