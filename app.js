const http = require('http');
const exec = require('node:child_process').exec;

const hostname = '127.0.0.1';
const port = 3000;

const server = http.createServer((req, res) => {
	
  exec("sqlite3 linkedboxdraw.db \".read select_document.sql\"", (error, stdout, stderr) => {
    console.log("STDOUT:", stdout, ", STDERR:", stderr);
	  res.statusCode = 200;
      res.setHeader('Content-Type', 'text/plain');
  //res.end('Hello World');
      res.end(stdout);
});

});

server.listen(port, hostname, () => {
  console.log(`Server running at http://${hostname}:${port}/`);
});