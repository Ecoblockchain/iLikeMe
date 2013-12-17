#! /usr/bin/env python

from sys import exit
from os import listdir, remove
from subprocess import Popen, PIPE
from string import lowercase
from random import choice, random
from re import match, sub
from time import time, sleep, strftime, localtime
from Queue import Queue
from xml.dom import minidom
from urllib2 import urlopen
from urllib import urlencode
from urlparse import parse_qs, urlparse
from BaseHTTPServer import BaseHTTPRequestHandler, HTTPServer
from nltk import pos_tag
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

def getPhrasesFromGoogle():
    phrases = []
    url = "http://google.com/complete/search?output=toolbar&"
    query = "everyone will %s"
    for letter in lowercase:
        xml = minidom.parseString(urlopen(url+urlencode({'q':query%letter})).read())
        for suggestion in xml.getElementsByTagName('suggestion'):
            phrase = sub(r'(will everyone)', 'everyone will', suggestion.attributes['data'].value)
            taggedText = pos_tag(phrase.split())
            sawVerb = False
            phrase = ''
            for (word,tag) in taggedText:
                sawVerb |= tag.startswith('VB') and not (('please' in word) or ('everyone' in word))
                phrase += word+' ' if sawVerb else ''
            if not phrase == '':
                phrases.append(sub(r'(lyric(s*))|(quote(s*))', '', phrase))
    return phrases

def setupOneApp(secrets):
    secrets['REDIRECT_URI'] = 'http://127.0.0.1:8080/'

    class FacebookRequestHandler(BaseHTTPRequestHandler):
        def do_GET(self):
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
     
            code = parse_qs(urlparse(self.path).query).get('code')
            code = code[0] if code else None

            if code is None:
                self.wfile.write("Sorry, authentication failed.")
                return

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

def setup():
    oauthDom = minidom.parse('./data/oauth.xml')
    graphs = Queue()
    for app in oauthDom.getElementsByTagName('app'):
        secrets = {}
        secrets['APP_ID'] = app.attributes['app_id'].value
        secrets['APP_SECRET'] = app.attributes['app_secret'].value
        graphs.put(facebook.GraphAPI(setupOneApp(secrets)))
    return graphs

def loop():
    oldAlbum = None
    for f in filter(lambda x: match('^img\.[\w]+\.png$', x), listdir(IMG_DIR)[0:5]):
        graph = graphs.get()
        message = "\"In the future, everyone will %s for 15 minutes.\"\n\n--%s"
        message %= (choice(phrases), str(graph.get_object("me")['name']))
        if (oldAlbum is not None) and (random() < 0.4):
            album = oldAlbum
        else:
            album = graph.put_object("me", "albums", name=str(f), message="me")
            oldAlbum = album
        imgFile = open(IMG_DIR+"/"+f)
        photo = graph.put_photo(image=imgFile, message=message, album_id=int(album['id']))
        graph.put_object(photo['id'], "likes")
        graph.put_object(photo['post_id'], "likes")
        imgFile.close()
        remove(IMG_DIR+"/"+f)
        graphs.put(graph)

if __name__ == '__main__':
    phrases = getPhrasesFromGoogle()
    graphs = setup()
    Popen('./iLikeMe.app/Contents/MacOS/iLikeMe', stdout = PIPE, stderr = PIPE)
    startTime = time()

    try:
        while(time()-startTime < 900):
            ## keep it from looping faster than ~60 times per second
            loopStart = time()
            loop()
            loopTime = time()-loopStart
            if (loopTime < 0.016):
                sleep(0.016 - loopTime)
        print "Your 15 minutes are over..."
        exit(0)
    except KeyboardInterrupt :
        exit(0)
