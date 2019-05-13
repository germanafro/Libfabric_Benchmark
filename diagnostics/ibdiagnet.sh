#!/bin/bash

ibdiagnet -r
ibcongest -f /var/tmp/ibdiagnet2/ibdiagnet2.fdbs -l /var/tmp/ibdiagnet2/ibdiagnet2.lst -a -Q ibcongest_AvA.dump
ibcongest -f /var/tmp/ibdiagnet2/ibdiagnet2.fdbs -l /var/tmp/ibdiagnet2/ibdiagnet2.lst -s ./myschedule -Q ibcongest_custom.dump