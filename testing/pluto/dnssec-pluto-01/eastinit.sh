/testing/guestbin/swan-prep
ipsec _stackmanager start 
/usr/local/libexec/ipsec/pluto --config /etc/ipsec.conf 
/testing/pluto/bin/wait-until-pluto-started
echo 192.1.2.23 east-from-hosts-file.example.com east-from-hosts-file >> /etc/hosts
ipsec auto --add westnet-eastnet
ipsec auto --status
echo "initdone"
