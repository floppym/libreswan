/testing/guestbin/swan-prep --46
ipsec _stackmanager start 
/usr/local/libexec/ipsec/pluto --config /etc/ipsec.conf 
/testing/pluto/bin/wait-until-pluto-started
ipsec auto --add v6-tunnel-east-road
ipsec auto --status
echo "initdone"
