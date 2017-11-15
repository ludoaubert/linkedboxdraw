import re
import os
import json
import socket
from subprocess import Popen, PIPE

HOST, PORT = '', 8080

listen_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
listen_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
listen_socket.bind((HOST, PORT))
listen_socket.listen(1)
print ('Serving HTTP on port %s ...' % PORT)
while True:
    client_connection, client_address = listen_socket.accept()
    request = client_connection.recv(1024)
    print(str(request))
    m = re.search(r'GET /getFilter\?([^ ]*) HTTP', str(request))
    if m:
        data = m.group(1)
        nr_rects = int(data[:3], 16)
        print('nr_rects')
        print(str(nr_rects))
        rectdim = data[3:3+nr_rects*6]
        nr_links = int(data[3+nr_rects*6:3+nr_rects*6+3],16)
        print('nr_links')
        print(str(nr_links))
        links = data[3+nr_rects*6+3:3+nr_rects*6+3+nr_links*6]
        filtre = data[3+nr_rects*6+3+nr_links*6:]
        print(rectdim)
        print(links)
        print(filtre)

#"http_get_param":"01408d0280a203804006807807808d03810b04804708804e0480d304808505806a06806907803903803f04809a0580620380700380700480320380c407801700100e00200e00200500300400501100600e00600500700000701000800900900f00a01200a01000a00900b00d00c00e00e00701000301100701300a01300701300801300b"
#http://localhost:8080/getFilter?0110af0880940180550180b00180940280850580a10880a20880940380b70180940180c40d811118808603807806806303807805801200300100c00b00c00800e00800d00800a00800a00900100000700800700500700600700200700100700200900700f00801000f002004ffff1

    command=['/home/eddi/Téléchargements/latuile', '--rectdim', rectdim, '--links', links, '--filter', filtre, '--reqkind', 'getFilter']
    print(str(command))
    proc = Popen(command, stdout=PIPE)
    json1 = ""
    for line in proc.stdout:
       json1 += line.decode('ascii')
    print(json1)

    rectdim = [rectdim[6*i:6*i+6] for i in range(nr_rects)]
    print('rectdim')
    print(str(rectdim))
    links = [(int(links[6*i:6*i+3],16), int(links[6*i+3:6*i+6],16)) for i in range(nr_links)]
    print('links')
    print(str(links))

    data = json.loads(json1)
    for context in data['contexts']:
        frame = context['frame']
        print('frame')
        print(str(frame))
        frame="{:04x}{:04x}{:04x}{:04x}".format(frame['left'],frame['right'],frame['top'],frame['bottom'])
        print('frame')
        print(frame)
        id_x_y = [(translatedBoxes['id'], translatedBoxes['translation']['x'], translatedBoxes['translation']['y']) for translatedBoxes in context['translatedBoxes']]
        print('id_x_y')
        print(str(id_x_y))
        translations = "".join("{:03x}{:03x}".format(x,y) for id,x,y in id_x_y)
        print(translations)

        idmap={}
        for id,x,y in id_x_y:
            idmap[id] = len(idmap)
        print('idmap')
        print(idmap)

        reverse_idmap = {v:k for k,v in idmap.items()}
        print('reverse_idmap')
        print(reverse_idmap)

        rectdim_ = "".join([rectdim[id] for id,x,y in id_x_y])
        print('rectdim_')
        print(rectdim_)
        assert(len(rectdim_)==len(translations))
        print('translations')
        print(translations)

        links_ = "".join(["{:02x}{:02x}".format(idmap[s],idmap[t]) for s,t in links if s in idmap and t in idmap])
        print('links_')
        print(links_)

        command=['/home/eddi/Téléchargements/bombix','--frame', frame,'--rectdim', rectdim_,'--translations', translations,'--links', links_]
        print(str(command))
        proc = Popen(command, stdout=PIPE)
        json2 = ""
        for line in proc.stdout:
           json2 += line.decode('ascii')
        print('json2')
        print(json2)
        polylines = json.loads(json2)
        for polyline in polylines:
            for u in ['from','to']:
                polyline[u] = reverse_idmap[ polyline[u] ]
        context['links'] = polylines

    print('contexts')
    print(json.dumps(data))

#    http_response = b"""\
#HTTP/1.1 200 OK
#
#Hello, World!
#"""
    http_response = bytearray(json.dumps(data),'ascii')
    client_connection.sendall(http_response)
    client_connection.close()
