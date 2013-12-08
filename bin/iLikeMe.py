#! /usr/bin/env python

from sys import exit
from os import listdir, remove
from re import match
from time import time, sleep, strftime, localtime
from urllib2 import urlopen
from urllib import urlencode
from urlparse import parse_qs, urlparse
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
import webbrowser
import facebook

IMG_DIR = './data/imgs'

def get_url(path, args=None):
    endpoint = 'graph.facebook.com'
    args = args or {}
    if 'access_token' in args or 'client_secret' in args:
        endpoint = "https://"+endpoint
    else:
        endpoint = "http://"+endpoint
    return endpoint+path+'?'+urlencode(args)

def get(path, args=None):
    return urlopen(get_url(path=path, args=args)).read()

def setup():
    secrets = {}
    secrets['REDIRECT_URI'] = 'http://127.0.0.1:8080/'

    inFile = open('./data/oauth.txt', 'r')
    for line in inFile:
        (k,v) = line.split()
        secrets[k] = v

    class FacebookRequestHandler(BaseHTTPRequestHandler):
        def do_GET(self):
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
     
            code = parse_qs(urlparse(self.path).query).get('code')
            code = code[0] if code else None

            if code is None:
                self.wfile.write("Sorry, authentication failed.")
                exit(1)

            response = get('/oauth/access_token', {'client_id':secrets['APP_ID'],
                                                   'redirect_uri':secrets['REDIRECT_URI'],
                                                   'client_secret':secrets['APP_SECRET'],
                                                   'code':code})
            secrets['ACCESS_TOKEN'] = parse_qs(response)['access_token'][0]

            self.wfile.write("You have successfully logged in to facebook. "
                             "You can close this window now.")


    httpd = HTTPServer(('127.0.0.1', 8080), FacebookRequestHandler)

    print "Logging you in to facebook..."
    webbrowser.open(get_url('/oauth/authorize', {'client_id':secrets['APP_ID'],
                                                 'redirect_uri':secrets['REDIRECT_URI'],
                                                 'scope':'read_stream,publish_actions,publish_stream,photo_upload,user_photos'}))

    while not 'ACCESS_TOKEN' in secrets:
        httpd.handle_request()

    return secrets['ACCESS_TOKEN']

def loop():
    for f in filter(lambda x: match('^img\.[\w]+\.jpg$', x), listdir(IMG_DIR)):
        album = graph.put_object("me", "albums", name=str(f), message=" ")
        imgFile = open(IMG_DIR+"/"+f)
        photo = graph.put_photo(image=imgFile, message=" ", album_id=int(album['id']))        
        graph.put_object(photo['id'], "likes")
        graph.put_object(photo['post_id'], "likes")
        imgFile.close()
        remove(IMG_DIR+"/"+f)

if __name__ == '__main__':
    graph = facebook.GraphAPI(setup())
    ## TODO: start oF app

    try:
        while(True):
            ## keep it from looping faster than ~60 times per second
            loopStart = time()
            loop()
            loopTime = time()-loopStart
            if (loopTime < 0.016):
                sleep(0.016 - loopTime)
    except KeyboardInterrupt :
        exit(0)

