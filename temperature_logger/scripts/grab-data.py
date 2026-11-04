#!/usr/local/bin/python3

#   ==============================================================================
#   Discover our devices using uPnP and query them as necessary
#
#   usage: grab-data.py [-f]
#
#   SCPD and SOAP stages are skipped, for now, as we know our devices capabilities
#
#   Devices are polled every 2 minutes. Each active device is then queried
#   and its responses are accumulated.  Up to seven days of data per device are kept
#
#   Format of ascii database files: <secs since epoch> , <temp> [ , <flag(s)> ] \n
#
#   Format of json message from server: 

import os, sys, subprocess, struct, glob
import socket, time, zlib
import urllib.request, urllib.error
import json

devices = dict()

# t is integer secs since epoch in UTC

def showtime(t):
    return time.strftime('%y/%m/%d %H:%M', time.localtime(t))


# =============================================================================
# find devices, create device dict entries

def get_wifi_strength(host):
    try:
        r = urllib.request.urlopen(host+'/getwifistrength').read().decode('ascii')
    except Exception:           #except (urllib.error.HTTPError, http.client.BadStatusLine):?
        r = '-'
    return r

def get_version(host):
    try:
        r = urllib.request.urlopen(host+'/version').read().decode('ascii')
    except Exception:           #except (urllib.error.HTTPError, http.client.BadStatusLine):?
        r = '0'
    return r

def get_last_checksum(host):
    try:
        r = urllib.request.urlopen(host+'/getrawfilecrc32').read().decode('ascii')
    except Exception:           #except (urllib.error.HTTPError, http.client.BadStatusLine):?
        r = '0'
    return r

    
def poll_using_upnp():
    upnpmsg = \
        'M-SEARCH * HTTP/1.1\r\n' \
        'HOST:239.255.255.250:1900\r\n' \
        'ST:upnp:rootdevice\r\n' \
        'MX:2\r\n' \
        'MAN:"ssdp:discover"\r\n' \
        '\r\n'

    max_tries = 5
    for i in range(max_tries):

        # Set up UDP socket
        sok = socket.socket(socket.AF_INET, socket.SOCK_DGRAM, socket.IPPROTO_UDP)
        sok.settimeout(2)
        sok.sendto(bytes(upnpmsg, 'utf-8'), ('239.255.255.250', 1900) )

        try:
            print('polling...')
            while True:
                data, addr = sok.recvfrom(65507)                # poll call
                data = data.decode('utf-8')
                #print(addr, data, '\n')
                # if device matches 'HAW Temperature Monitor' capture name
                # add name to table or update table entry with new address
                # mark address live
                v = dict(s.split(':', 1) for s in data.splitlines() if ':' in s)
                ##print(v)
                if not 'SERVER' in v or not 'LOCATION' in v: continue
                key = 'HAW Temperature Monitor ('
                r = v['SERVER'].find(key)
                if r == -1: continue
                s = v['SERVER'][r+len(key):]
                t = s.find(')')
                if t == -1: continue
                name = s[0:t]
                host = v['LOCATION'].strip()
                host = host[:host.rfind('/')]
                version = get_version(host)
                strength = get_wifi_strength(host)
                print('found: ', name, host, version, strength)
                ##if version != '0':
                if not name in devices:
                    devices[name] = [host, [-100, -100, 0, 0, 0], [-100, -100, 0, 0, 0]]              # min, max, last, startt, endt
                else:
                    devices[name][0] = host
        except socket.timeout:
            pass
        if len(devices) > 0: break
    print('ending poll')
    #sys.exit(0)                        # stop after poll



# update maxima & minima
def update_statistics(n, k, t, temp):
    #print('update_statistics: '+str(n)+', '+str(k)+', '+str(t)+', '+str(temp))
    devices[k][n][2] = temp                                                             # latest (end) temp
    if devices[k][n][0] == -100 or temp < devices[k][n][0]: devices[k][n][0] = temp     # minimum temp
    if devices[k][n][1] == -100 or temp > devices[k][n][1]: devices[k][n][1] = temp     # maximum temp
    if devices[k][n][3] == 0: devices[k][n][3] = t                                      # start time
    devices[k][n][4] = t                                                                # latest (end) time

# =============================================================================
# old style, get data line by line in ascii

# DEPRECATED
# create file in db directory if non-existent & return file open for appending
#
projectsdir = "/a-stuff/a-projects"
dbasedir = projectsdir+"/zoning-system/database/"
#
# DEPRECATED
def get_open_device_file(k):
    fname = os.path.expanduser('~') + dbasedir + k
    if (not os.path.exists(fname)):      
        f = open(fname, 'w')
        f.close()
    return open(fname, 'a')


# DEPRECATED
# get a json load of data from device 'v' as an array
def get_data_from_host(v0):
    try:
        contents = urllib.request.urlopen(v0+'/sendtemperatures').read()
    except urllib.error.URLError:
        return []
    jdata = json.loads(contents)
    return jdata['timetempflagdata']

# DEPRECATED
# write record to db as a single line
def write_db_record(f, s):
    f.write(s+'\n')

# DEPRECATED
# convert time reported by device to seconds since epoch
def calculate_time(tnow, flags, treported):
    if flags & 3 == 3:
        t = treported                                   # time is a unix timestamp in seconds
    else:
        t = int(round(tnow - int(treported/1000)))     # time is in millisecs before now
    return t

# flags:
#     bottom 2 bits:  0 - old data, 1 - no ntp time, 2 - ntp time available, 3 - time reported is unix ts in seconds

# DEPRECATED
# get json data from a device(s), process, write to db
def get_device_data(timenow, k, v):
    d = get_data_from_host(v[0])
    if len(d) == 0: return 0
    f = get_open_device_file(k)
    #print(json.dumps(d))
    for i in range(int(len(d)/3)):
        flags = d[i*3]
        t = calculate_time(timenow, flags, d[i*3+1])                # convert to secs since epoch
        temp = d[i*3+2]                                             # float temperature
        if i==0:
            print(showtime(t) + ':    ', end="")                      # display time at beginning of each line
        print(str(temp) + ', ', end="")                               # display each temperature
        write_db_record(f, str(t)+','+str(temp)+','+str(flags))
        update_statistics(2, k, t, temp)
    print(' ')
    f.close()
    return len(d) >= 18         # more to do

    
# DEPRECATED
# get json data from all device(s) & process
def get_all_active_device_data():
    timenow = time.time()
    continuing = 0
    for k, v in devices.items():
        if get_version(v[0]) == '0':
            r = get_device_data(timenow, k, v)
            if r: continuing = 1
    return continuing


# DEPRECATED
# now do a "suck-data.py"
def suck_up_data():
    while(1):                       # do this every few seconds
        r = get_all_active_device_data()    
        if not r: break
        time.sleep(1)               # but flush quickly if full

# =============================================================================
# new style get data from server file system on device(s)

# n.b. commands.getOutput is not python 3 compatible; need to use subprocess instead

# get list of server data files, trim away leading /, eliminate all in local files except latest

localfileroot = projectsdir+'/zoning-system/temperature-logger/scripts/rawdata'
localfiledir  = os.path.expanduser('~') + localfileroot
datafilepfx   = 'tdatafile'
dbdir         = os.path.expanduser('~') + dbasedir

def run_cmd(cmd):
    cargs = cmd.split()
    try:
        op = subprocess.check_output(cargs)
    except subprocess.CalledProcessError as ex:
        print('command "'+cmd+'" returned code '+str(ex.returncode), file=sys.stderr)
        print(ex.output[:80], file=sys.stderr)
        sys.exit(1)
    return op

def find_server_files_needed(name, host):
    print("sync... " + run_cmd('curl -f '+host+"/sync").decode('utf-8'))            # force host to write out data to file system
    spflist = run_cmd('curl -f '+host+"/listspiffstoclient").decode('utf-8')        # get list of files on server
    ##print(">>>" + spflist + "<<<")
    spfarray1 = [f.split() for f in spflist.splitlines() if len(f.split()) == 2]    # get just the files
    #print(spfarray1)
    # restrict to data files, & just names, also convert length to number of lines in equivalent text file
    spfarray2 = [[f[1][1:],int(f[0])/12] for f in spfarray1 if f[1].startswith('/'+datafilepfx)]
    if len(spfarray2) == 0: spfarray2 = [[f[1][0:],int(f[0])/12] for f in spfarray1 if f[1].startswith(datafilepfx)]
    #print(spfarray2)
    lcllist = run_cmd('ls '+localfiledir+'/'+name).decode('utf-8')                  # get list of files in rawdata dir
    lclarray1 = [f.replace(".txt", "") for f in lcllist.splitlines() if f.startswith(datafilepfx)]      # restrict to datafiles, lose suffix
    wntarray1 = [f for f in spfarray2 if not f[0] in lclarray1]                     # files on client that we don't have
    # files on client that we do have, but whose length doesn't match
    wntarray2 = [f for f in spfarray2 if f[0] in lclarray1 and f[1] != int(run_cmd('wc -l '+localfiledir+'/'+name+'/'+f[0]+'.txt').split()[0])]
    #print('dbg ', spfarray2[0][1], ' ', run_cmd('wc -l '+localfiledir+'/'name+'/'+spfarray2[0][0]+'.txt').split()[0])
    wntarray = [f[0] for f in wntarray1 + wntarray2]
    wntarray.sort()
    return wntarray

# convert files from a contiguous sequence of qentry's to comma separated ascii
#
#   struct qentry {
#       unsigned long ptime;
#       float temp;
#       unsigned char flags;
#   };

def get_and_convert_file(name, host, fnm):
    localname = localfiledir+'/'+name+'/'+fnm+".txt"
    locallen = 0
    if os.path.exists(localname): 
        locallen = int(run_cmd('wc -l '+localname).strip().split()[0])
    fdata = run_cmd('curl --keepalive-time 120 -f '+host+"/getfile?f="+fnm)         # get the data
    #print(struct.calcsize('=LfxBxx'))
    format = '=LfBxxx'
    entsize = struct.calcsize(format)
    if len(fdata) % entsize != 0:
        print('file, length '+str(len(fdata))+', read from device is not an exact multiple of qentrys', file=sys.stderr)
        sys.exit(1)
    crclcl = zlib.crc32(fdata)
    crclcl = f"{crclcl & 0xffffffff:08X}"
    crcrmt = get_last_checksum(host)
    #print(crcrmt)
    if crcrmt != '0':                           # ignore checksums if getrawfilecrc32 is not implemented
        if crclcl != crcrmt:
            print('checksums '+crclcl+' and '+crcrmt+' do not match!')
            sys.exit(1)
    print('length of "'+fnm+'" is '+str(len(fdata)))
    entries = int(len(fdata) / entsize)
    #print('file '+fnm+', locallen = '+str(locallen)+', entries = '+str(entries))
    f = open(localname, 'w')
    for i in range(entries):
        (ptime, temp, flags) = struct.unpack_from(format, fdata, i * entsize)
        print(str(ptime)+','+('%.6f' % temp)+','+str(flags), file=f)
        if i >= locallen:
            update_statistics(1, name, ptime, temp)
    f.close()
    #print('ptime='+str(ptime)+', temp='+str(temp)+', flags='+str(flags))

def concatenate_files_to_db(name):
    ofname = dbdir+'z-'+name+"-retrieved"
    outfile = open(ofname, "w")
    fnlist = glob.glob(localfiledir+'/'+name+'/'+datafilepfx+'*')
    fnlist.sort()
    for fname in fnlist:
        with open(fname, "r") as f:
            for line in f:
                outfile.write(line)
    outfile.close()
    print("catenated... " + run_cmd('wc -l '+ofname).decode('utf-8'))

def get_device_data_files(name, details):
    print(name + " (" + details[0] + ")")
    files_to_get = find_server_files_needed(name, details[0])
    print("files to get: ", files_to_get)
    for fnm in files_to_get:
        get_and_convert_file(name, details[0], fnm)
        #sys.exit(0);                                       # stop after processing just one file
    concatenate_files_to_db(name)

def show_statistics(n):
    for k, v in devices.items():
        if (v[n][0] != -100):
            print(k, ' -  min: ', v[n][0],',  max: ', v[n][1], ',  current: ', v[n][2], end="")
            print(',  start: ', showtime(v[n][3]), ',  end: ', showtime(v[n][4]))
        else:
            print(k, ' - no data')


# main loop

# nothing to do with grab-data.py at all!
# nb: can't use -n and -f together

def check_other_stuff():
    if len(sys.argv) < 2 or sys.argv[1] != '-n':
        if (os.path.exists("check-other-stuff.py")):
            print(run_cmd("check-other-stuff.py").decode('utf-8').strip())


def main():
    check_other_stuff()    
    count = 0;
    while(1):
        if count == 0:
            poll_using_upnp()           # do this first time & every 10 mins
        count += 1
        if count >= 30: count = 0
        ##sys.exit(0)
        
        for k, v in devices.items():
            get_device_data_files(k, v)
        show_statistics(1)

        ##sys.exit(0)                    # don't bother with second method of acquiring data
        
        
        # DEPRECATED
        print('')
        suck_up_data()
        show_statistics(2)

        if len(sys.argv) < 2 or sys.argv[1] != '-f': break      # once through vs repeatedly
        time.sleep(20)

main()
