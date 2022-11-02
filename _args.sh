#!/bin/bash

sudo insmod Args.ko mystring="bebop" myintarray=-1,-4
sudo dmesg -H | tail -8

sudo rmmod Args
sudo dmesg -H | tail -1
