#!/usr/bin/env bash

mydir=$(dirname $(realpath $BASH_SOURCE))
srcdir=$(dirname $mydir)

set -x

render () {
    local name="$1" ; shift
    local What="$1" ; shift
    local outdir="${1:-$srcdir/src/${name}}"
    local what="$(echo $What | tr '[:upper:]' '[:lower:]')"
    local tmpl="o${what}.hpp.j2"
    local outhpp="$outdir/${What}.hpp"
    mkdir -p $outdir
    moo -g '/lang:ocpp.jsonnet' -M $mydir  \
        -A path="dunedaq.readout.${name}" \
        -A ctxpath="dunedaq.readout" \
        -A os="readout-${name}-schema.jsonnet" \
           render omodel.jsonnet $tmpl \
           > $outhpp
    echo $outhpp
}

render rdlg Structs
render rdlg Nljs

