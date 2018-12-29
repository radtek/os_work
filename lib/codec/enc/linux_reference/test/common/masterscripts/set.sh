#!/bin/bash
#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Hantro Products Oy.                             --
#-                                                                            --
#-                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-                                                                            --
#--   Abstract     : Master script which cleans the directory, sets the       --
#                    software variables, compiles the software and            --
#                    test benches, and copies all scripts and binaries to test--
#                    directory                                                --
#--                                                                           --
#-------------------------------------------------------------------------------

#=====--------------------------------------------------------------------=====#

#Cleans the testing directory
echo -n "Delete all files from previous testing? [y/n] "
read ans
if [ "$ans" == "y" ]
then
    ./del.sh
fi

#Changes the software files according to the configuration
./setupconfig.sh
#Compiles the memalloc and kernel drivers, and all the test benches
./make_versatile.sh
#Copies the necessary binaries to the test directory
./copy_script
