#!/usr/bin/env python3

# A minimal HTTP server that temporarily saves submitted files
# in a temporary folder and serves them back to workers
# usage:
#       $ python file_server.py [tasks_dir]
# 
# If "files_dir" is specified, the server scans this directory
# on startup and stores its files in the "tasks" hash storage

import http.server as http
import socketserver
import cgi
import os
import sys
import tempfile
import tarfile
import hashlib

join = os.path.join

port = 9999
tmp = tempfile.TemporaryDirectory()

# Read command line arguments
argv_iter = iter(sys.argv[1:])
task_source = ""

for arg in argv_iter:
    task_source = arg

# Make the file structure of the file server
archive_dir = join(tmp.name, "submit_archives")
os.makedirs(archive_dir)

submit_dir = join(tmp.name, "submits")
os.makedirs(submit_dir)

result_dir = join(tmp.name, "results")
os.makedirs(result_dir)

task_dir = join(tmp.name, "tasks")
os.makedirs(task_dir)

# An iterator for the supplementary task files
def walk_files ():
    for root, dirs, files in os.walk(task_source):
        for name in files:
            yield os.path.join(root, name)

# Read and store the supplementary task files
if task_source:
    print("Loading files from {0}...".format(os.path.realpath(task_source)))

    for taskfile_name in walk_files():
        if os.path.isfile(taskfile_name):
            with open(taskfile_name, "rb") as taskfile:
                content = taskfile.read()
                destfile_name = hashlib.sha1(content).hexdigest()
                path = join(task_dir, destfile_name[0])
                os.makedirs(path, exist_ok = True)

                with open(join(path, destfile_name), "wb") as destfile:
                    destfile.write(content)

                rel = os.path.relpath(taskfile_name, task_source)
                print("{0}: {1}".format(destfile_name, rel))

# An id for new jobs
job_id = 0

# Change to the temporary directory
os.chdir(tmp.name)

class FileServerHandler(http.SimpleHTTPRequestHandler):
    def do_POST(self):
        form = cgi.FieldStorage(
            fp = self.rfile,
            headers = self.headers,
            environ = {
                'REQUEST_METHOD': 'POST',
                'CONTENT_TYPE': self.headers['Content-Type']
            }
        )

        global job_id
        job_id += 1
        job_dir = join(submit_dir, str(job_id))
        os.makedirs(job_dir)

        # Save received files
        for name in form.keys():
            dirname = os.path.dirname(name)
            if dirname != "":
                os.makedirs(join(job_dir, dirname), exist_ok = True)
            
            with open(join(job_dir, name), "w") as f:
                f.write(form[name].value)

        # Make an archive from the submitted files
        archive_path = join(archive_dir, str(job_id) + ".tar.gz")
        with tarfile.open(archive_path, "w:gz") as archive:
            archive.add(job_dir, arcname = str(job_id))

        # Response headers
        self.send_response(200)
        self.end_headers()

        # Return the job id assigned to submitted files
        self.wfile.write(str(job_id).encode())

server = socketserver.TCPServer(("", port), FileServerHandler)

print("Serving files from {0} at port {1}...".format(tmp.name, port))
server.serve_forever()