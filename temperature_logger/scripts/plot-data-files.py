#!/usr/local/bin/python3

import os, sys, time, queue
import matplotlib.pyplot as plt 
import matplotlib.dates as mdates
from datetime import datetime
import numpy as np

import matplotlib
print('= '+matplotlib.matplotlib_fname()+' =')

projectsdir = "/a-stuff/a-projects"
dbasedir = projectsdir+"/zoning-system/database/"
specialfile = "kitchen"
altname = "dw fan"
nospecial = True
showfan = False
showonratio = False
processalldates = False
starttime = time.time() - 32*24*60*60   # default is show up to one month back
sampleqlength = 6                       # hours

# usage: plot-data-files.py [-a] [-#[#]] [-z] [-f] [-r] [file]
#
# -a:  include all available dates in plot (slow)
# -z:  include the "special" file ("kitchen" - which has non-spiffs data in it)
# -f:  show fan on times
# -r:  graph 3 hour fan on time ratio
# -##: go back so many months instead of default 1 month

tgtfnm = ''
while len(sys.argv) > 1:
    if not sys.argv[1].startswith('-'):
        break
    if sys.argv[1] == '-a':
        processalldates = True
        sys.argv.pop(1)
        continue
    if sys.argv[1] == '-f':
        showfan = True
        sys.argv.pop(1)
        continue
    if sys.argv[1] == '-r':
        showonratio = True
        sys.argv.pop(1)
        continue
    if sys.argv[1][0] == '-' and sys.argv[1][1:].isdigit():
        starttime = time.time() - int(sys.argv[1][1:])*30*24*60*60   # show n months back
        sys.argv.pop(1)
        continue
    if sys.argv[1] == '-z':
        nospecial = not nospecial
        sys.argv.pop(1)
        continue

if len(sys.argv) > 1:
    tgtfnm = sys.argv[1]

if (processalldates):
    starttime = time.mktime((2018, 1, 1, 0, 0, 0, 0, 0, 0))

fig, ax1 = plt.subplots()
ax1.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d %H:%M'))
plt.title('Temperatures against Time') 
plt.xlabel('time')
if showonratio:
    ax2 = ax1.twinx()
    ax2.xaxis.set_major_formatter(mdates.DateFormatter('%m-%d %H:%M'))
    ax2.set_ylabel('fan on ratio')
ax1.set_ylabel('temperature in celsius')
plt.ioff()              # turn off interactive mode (does nothing?)
plt.subplots_adjust(left=0.07, right=0.95, top=0.9, bottom=0.1)

dir = os.path.expanduser('~') + dbasedir
filelist = os.listdir(dir)
filelist.sort()

#dbprt = 0

for fnm in filelist:
    if os.path.isdir(dir+fnm): continue
    if tgtfnm != '' and not fnm.startswith(tgtfnm): continue
    if tgtfnm == '' and fnm.startswith('.'): continue
    if tgtfnm == '' and fnm.startswith('X'): continue
    if nospecial and fnm == specialfile: continue

    if showfan or showonratio:
        if nospecial or fnm == specialfile:
            print(altname)
            f = open(dir+fnm)
            x, y, z, lno, lastx = [], [], [], 0, 0
            onsampcnt, samplecnt, smpqueue = 0, 0, queue.Queue(maxsize=sampleqlength*60)    # samples come 1 per minute
            for line in f:
                flags = 0
                lno += 1
                try:
                    s, t = line.rstrip('\n').split(',',1)
                except ValueError:
                    print("ValueError in file '",fnm, "' line ", lno, ":  ", line)
                if float(s) < starttime: continue
                if t.find(',') >= 0:
                    try:
                        t, flags = t.split(',')
                    except ValueError:
                        print("ValueError - too many commas - in file '",fnm, "' line ", lno, ":  ", line)
                ts = float(s)
                if ts - lastx > 15*60:              # suppress graphing during gaps
                    x.append(datetime.fromtimestamp(lastx+60))
                    y.append(np.nan)
                    z.append(np.nan)
                lastx = ts
                x.append(datetime.fromtimestamp(ts))
                if showfan:
                    if int(flags) & 8 == 8:             # was fan on
                        y.append(float(t))
                    else:
                        y.append(np.nan)                # suppress graphing when fan is not on
                if showonratio:
                    samplecnt += 1                      # calculate running average
                    if smpqueue.full():
                        oval = smpqueue.get()
                        onsampcnt -= oval
                        samplecnt -= 1
                    sample = 0
                    if int(flags) & 8 == 8:
                        sample = 1
                        onsampcnt += 1
                    smpqueue.put(sample)
                    zval = 100.0 * float(onsampcnt) / float(samplecnt)
                    if zval < 0.01: zval = np.nan       # don't include baseline in graph
                    z.append(zval)
                    #if sample == 1 and dbprt == 0: dbprt = 1
                    #if dbprt > 600 and dbprt <= 1900:
                    #    print oval, sample, onsampcnt, samplecnt, zval
                    #if dbprt > 0:
                    #    dbprt += 1
            f.close()
            #plt.ylim(15.0, 25.0)
            if showfan:
                ax1.plot(x, y, label=altname)
            if showonratio:
                ax2.plot(x, z, label="fan ratio", color='tab:red', linewidth=0.5)
        showfan, showonratio = False, False

    print(fnm)
    f = open(dir+fnm)
    x, y, lno, lastx = [], [], 0, 0
    for line in f:
        lno += 1
        try:
            s, t = line.rstrip('\n').split(',',1)
        except ValueError:
            print("ValueError in file '",fnm, "' line ", lno, ":  ", line)
        if float(s) < starttime: continue
        ts = float(s)
        if ts - lastx > 10*60:                      # if there is a gap, suppress pt with nan
            x.append(datetime.fromtimestamp(lastx+60))
            y.append(np.nan)
        lastx = ts
        x.append(datetime.fromtimestamp(ts))
        if t.find(',') >= 0:
            try:
                t, flags = t.split(',')
            except ValueError:
                print("ValueError - too many commas - in file '",fnm, "' line ", lno, ":  ", line)
        y.append(float(t))
        #print(time.asctime(time.localtime(float(s)))+'   '+t)
    f.close()
    #plt.ylim(15.0, 25.0)
    if fnm.startswith('z'):
        ax1.plot(x, y, label=fnm, linewidth=0.5)
    else:
        ax1.plot(x, y, label=fnm)
        

plt.legend()
plt.get_current_fig_manager().toolbar.zoom()   # https://stackoverflow.com/questions/8144492/wonder-twin-power-zoom-to-rectangle-activate
#fig.tight_layout()     # overrides subplots_adjust() if used
plt.show() 
