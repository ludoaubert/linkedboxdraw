const http = require('http');
const exec = require('node:child_process').exec;

const hostname = '127.0.0.1';
const port = 3000;

const SQLITE_TOOLS_DIR='C:/lulu/sqlite-tools-win32-x86-3410200'

const server = http.createServer((req, res) => {

    if (req.url=='/list_diagrams')
    {
        exec(`${SQLITE_TOOLS_DIR}/sqlite3 linkedboxdraw.db "SELECT json_group_array(json_object('title',title, 'deleted', deleted, 'guid', guid)) FROM diagram"`,(error, stdout, stderr) => {
          console.log("STDOUT:", stdout, ", STDERR:", stderr);
	      res.statusCode = 200;
          res.setHeader('Content-Type', 'application/json');
          res.end(stdout);
		});
    }
	else
    {
        exec(`${SQLITE_TOOLS_DIR}/sqlite3 linkedboxdraw.db ".read select_document.sql"`, (error, stdout, stderr) => {
          console.log("STDOUT:", stdout, ", STDERR:", stderr);
	      res.statusCode = 200;
          res.setHeader('Content-Type', 'application/json');
  //res.end('Hello World');
          res.end(stdout);
        });
    }

});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});