#! /usr/bin/python
 
from hashlib import sha512
from hmac import HMAC
import base64
import argparse
import urlparse

def get_curl(token, method, url, secretkey, custom_header):
    path = urlparse.urlparse(url).path
    data = "%s-%s" % (path, custom_header)
    print data
    signature = base64.b64encode(str(HMAC(secretkey, data, sha512).hexdigest()))
    print 'curl -H "Authorization: NYTV %s:%s" -H "X-NYTV-Date: %s" %s -v' % (token, 
                                                                              signature,
                                                                              custom_header,
                                                                              url) 


parser=argparse.ArgumentParser()
class Data(object):
    pass

data = Data()
parser.add_argument('-m', help = 'HTTP METHOD (GET, POST, DELETE, PUT)', dest='method')
parser.add_argument('-u', help = 'URL', dest='url')
parser.add_argument('-c', help = 'CUSTOM DATE', dest='custom')
parser.add_argument('-t', help = 'TOKEN', dest='token')
parser.add_argument('-s', help = 'SECRETKEY', dest='secretkey')
args = parser.parse_args(namespace=data)

if __name__ == "__main__":
    get_curl(data.token, data.method, data.url, data.secretkey, data.custom)
