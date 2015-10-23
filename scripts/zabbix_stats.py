#!/usr/bin/python
"""
Shows a list of all current issues (AKA tripped triggers)
"""

from getpass import getpass
from pyzabbix import ZabbixAPI
from hurry.filesize import size
import numpy as np
import time
import pprint
import re
import ConfigParser
import getopt
import sys


def main(argv):

    cfgfile = "/etc/zabbix_stats.cfg"
    try:
        opts, args = getopt.getopt(argv, "hc:", ["configfile="])
    except getopt.GetoptError:
        print 'zabbix_stats.py -c <configfile>'
        sys.exit(2)
    for opt, arg in opts:
        if opt == '-h':
            print 'zabbix_stats.py -c <configfile>'
            sys.exit(2)
        elif opt in ("-c", "--configfile"):
            cfgfile = arg

    pp = pprint.PrettyPrinter(indent=4)
    config = ConfigParser.ConfigParser()
    config.read(cfgfile)

    global zapi
    zapi = ZabbixAPI(config.get('Server', 'zabbixurl'))
    zapi.session.verify = False
    # Login to the Zabbix API
    zapi.login(config.get('Server', 'username'), config.get('Server', 'password'))

    while(1):
        Alerts()    



def Stats():
    global zapi
    BWVals = {}
    results = {}
    BWVals['MikroTik'] = {'key':{'if*Octets[Wan2 - Client]', 'if*Octets[Wan1 - Server]'}, 'limit':10}
    BWVals['R510'] = {'key':{'system.cpu.util[,idle]', 'DellTemp[1]'}, 'limit':10}
    BWVals['nx5010'] = {'key':{'nexus.temp3','nexus.temp1'}, 'limit':1}
    BWVals['H2216-N3'] = {'key':{'system.cpu.util[,idle]', 'bb_inlet_temp', 'exit_air_temp'}, 'limit':10}
    BWVals['H2216-N2'] = {'key':{'system.cpu.util[,idle]'}, 'limit':10}
    BWVals['H2216-N1'] = {'key':{'system.cpu.util[,idle]'}, 'limit':10}
    
    for vals in BWVals:
        hosts = zapi.host.get(filter=({'name':vals}),
            output=['hostid', 'name'],
        )
        results[vals] = {}
        for key in BWVals[vals]['key']:
            items = zapi.item.get(hostids=hosts[0]['hostid'], 
                search=({"key_":key}),
                searchWildcardsEnabled=1,
                output=['name', 'key_'],
            )
            for t in items:
                results[vals][t['key_']] = {}
                asfloat=0
                results[vals][t['key_']]['values'] = zapi.history.get(itemids=t['itemid'],
                    limit=int(BWVals[vals]['limit']),
                        sortfield='clock',
                        sortorder='DESC',
                )
                if not len(results[vals][t['key_']]['values']):
                    results[vals][t['key_']]['values'] = zapi.history.get(itemids=t['itemid'],
                        limit=int(BWVals[vals]['limit']),
                        history=0,
                        sortfield='clock',
                        sortorder='DESC',
                    )
                values = []
#                pp.pprint(vals + " " + t['key_'])
                for s in results[vals][t['key_']]['values']:
#                    pp.pprint(time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(float(s['clock']))))
                    if re.match("system.cpu.util.*", t['key_']):
                        s['value'] = 100 - float(s['value'])
                    values.append(float(s['value']))
                results[vals][t['key_']]['average'] = np.average(values)

#    pp.pprint(results)

    stats = str("WAN Stats - Server: {0}bps/{1}bps - Client: {2}bps/{3}bps Server Load: R510 - {4:.0f}% H2216-N1 - {5:.0f}% H2216-N2 - {6:.0f}% H2216-N3 - {7:.0f}% Temps: Nexus - {8:.0f}/{9:.0f} R510 - {10:.0f} H2216 - {11:.0f}/{12:.0f}"
        .format(size(results['MikroTik']['ifInOctets[Wan1 - Server]']['average']),
        size(results['MikroTik']['ifOutOctets[Wan1 - Server]']['average']),
        size(results['MikroTik']['ifInOctets[Wan2 - Client]']['average']),
        size(results['MikroTik']['ifOutOctets[Wan2 - Client]']['average']),
        results['R510']['system.cpu.util[,idle]']['average'],
        results['H2216-N1']['system.cpu.util[,idle]']['average'],
        results['H2216-N2']['system.cpu.util[,idle]']['average'],
        results['H2216-N3']['system.cpu.util[,idle]']['average'],
        results['nx5010']['nexus.temp3']['average'],
        results['nx5010']['nexus.temp1']['average'],
        results['R510']['DellTemp[1]']['average'],
        results['H2216-N3']['bb_inlet_temp']['average'],
        results['H2216-N3']['exit_air_temp']['average'],
    ))
    f1=open('/tmp/test/message-2', 'w')
    if len(stats) > 0:
        f1.write(stats)
    f1.close()

def Alerts():
    # Get a list of all issues (AKA tripped triggers)
    global zapi
    triggers = zapi.trigger.get(only_true=1,
        skipDependent=1,
        monitored=1,
        active=1,
        output='extend',
        expandDescription=1,
        expandData='host',
        withLastEventUnacknowledged=1,
        sortfield='priority',
        sortorder='DESC',
    )

    prioritylist = ['N/A', 'Info', 'Warn', 'Average', 'High', 'Critical']
    errorlist = []
    for t in triggers:
        if (int(t['value']) == 1 and int(t['priority']) > 1):
            errorlist.append(str("Error:{0}, Priority:{1}".format(
                t['description'],
                prioritylist[int(t['priority'])])
            ))
    for i in range(0, len(errorlist),3):
        Stats()
        f1=open('/tmp/test/message-3', 'w')
        if i < len(errorlist):
            f1.write(errorlist[i])
        f1.close()
        f2=open('/tmp/test/message-4', 'w')
        if i+1 < len(errorlist):
            f2.write(errorlist[i+1])
        f2.close()
        f3=open('/tmp/test/message-5', 'w')
        if i+2 < len(errorlist):
            f3.write(errorlist[i+2])
        f3.close()
        time.sleep(60)
    


if __name__ == "__main__":
    main(sys.argv[1:])
