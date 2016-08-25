import socket
import select
import sys

timeout = 5000

def error(str):
    print '\033[1;31m%s\033[m' %(str)

def info(str):
    print '\033[1;36m%s\033[m' %(str)

def send(sock, s):
    sock.sendall(s)
    p = select.poll()
    p.register(sock, select.POLLIN)
    if not p.poll(timeout):
        error('send request to %s:%d timeout'%sock.getpeername())
        return None
    return sock.recv(1024).strip('\r\n').split(' ')

def parseConf(path):
    l = []
    f = open(path)
    for line in f:
        line = line.strip('\n')
        if not line:
            continue
        addr, port = line.split(':')
        l.append((addr, int(port)))
    return l

def checkAlive(l):
    info('- check node alive') 
    for addr in l:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        try:
            sock.connect(addr)
            print 'check node %s ok' %(repr(addr))
            sock.close()
        except socket.error, e:
            error('check node %s error:%s'%(repr(addr), e[1]))

def findLeader(l):
    for addr in l:
        try:
            print 'tring to connect %s:%d'%addr
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(addr)
            t = send(sock, 'ADD checkleader\r\n')
            if not t or t[0] != 'TRANS':
                sock.close()
                continue
                
            addr, port = t[1].split(':')
            return (addr, int(port))
        except socket.error, e:
            error('connect to %s error:%s'%(repr(addr), e[1]))

def checkleader(l, token):
    info('- check leader node')
    t = findLeader(l)
    if not t:
        error('check leader error:no leader')
        return
    addr, port = t
    print 'find leader:%s'%(repr((addr, port)))
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((addr, port))
    except socket.error, e:
        error('connect leader error:%s'%(e[1]))
    t = send(sock, 'ADD checkleader %s\r\n'%(token))
    if t and t[0] != 'OK':
        error('check create key error:%s'%(repr(t)))
    else:
        print 'check create key ok'

    t = send(sock, 'GET checkleader\r\n')
    if t and t[0] != 'ID':
        error('check get key error:%s'%(repr(t)))
    else:
        print 'check get key ok'
     
    t = send(sock,'DEL checkleader %s\r\n'%(token)) 
    if t and t[0] != 'OK':
        error('check delete key error:%s'%(repr(t)))
    else:
        print 'check delete key ok'
            
def main():
    if len(sys.argv) < 3:
        print "usage check.py server.conf token"
        return
    l = parseConf(sys.argv[1])
    checkAlive(l)
    checkleader(l, sys.argv[2])

if __name__ == '__main__':
    main()
