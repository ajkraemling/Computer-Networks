#!/bin/bash

servers=(
    "www.wpi.edu"
    "www.hmc.edu"
    "www.gonzaga.edu"
    "www.ucla.edu"
    "www.und.edu"
    "www.vuw.ac.nz"
    "www.ox.ac.uk"
    "abuacademy.com"
    "www.grantham.edu"
    "www.compton.edu"
    "www.ccri.edu"
    "www.csudh.edu"
    "www.brillarebeautyinstitute.edu"
    "www.bpc.edu"
    "www.bsc.edu"
    "www.asumidsouth.edu"
    "www.asub.edu"
    "www.aci.edu"
    "www.mcact.org"
    "www.ancollege.edu"
)

count=20
echo -n "Server,IP"
for i in $(seq 1 $count); do
    echo -n ",Ping$i"
done
echo

for server in "${servers[@]}"; do
    # strip protocol and trailing slashes
    clean_server=$(echo "$server" | sed -E 's~https?://~~' | sed 's:/$::')
    echo -n "$clean_server,"

    # First ping to get IP
    first_output=$(./webclient "$clean_server" -ping 2>/dev/null)
    ip=$(echo "$first_output" | awk '{print $1}')
    echo -n "$ip"

    # Store RTTs
    rtts=()
    rtts+=($(echo "$first_output" | awk '{print $3}'))

    for i in $(seq 2 $count); do
        output=$(./webclient "$clean_server" -ping 2>/dev/null)
        rtt=$(echo "$output" | awk '{print $3}')
        rtts+=("$rtt")
    done

    for rtt in "${rtts[@]}"; do
        echo -n ",$rtt"
    done
    echo
done

