#!/bin/sh -f

#Imports
.  ./f_check.sh

# Use the same time for all reports
export REPORTTIME=`date +%d%m%y_%k%M | sed -e 's/ //'`

importFiles

Usage="\n$0 [-csv]\n\t -csv generate csv file report\n"

resultcsv=result.csv

CSV_REPORT=0

csvfile=$csv_path/integrationreport_jpeg_${hwtag}_${swtag}_${reporter}_$reporttime

# parse argument list
while [ $# -ge 1 ]; do
        case $1 in
        -csv*)  CSV_REPORT=1 ;;
        -*)     echo -e $Usage; exit 1 ;;
        *)      ;;
        esac

        shift
done

if [ $CSV_REPORT -eq 1 ]
then
    echo "Creating report: $csvfile.csv"
    echo "TestCase;TestPhase;Category;TestedPictures;Language;StatusDate;ExecutionTime;Status;HWTag;EncSystemTag;SWTag;TestPerson;Comments;Performance;KernelVersion;RootfsVersion;CompilerVersion;TestDeviceIP;TestDeviceVersion" >> $csvfile.csv
fi

first_case=2000
last_case=2999
sum_ok=0
sum_notrun=0
sum_failed=0

for ((case_nr=$first_case; case_nr<=$last_case; case_nr++))
do
    let "set_nro=${case_nr}-${case_nr}/5*5"

    . $test_case_list_dir/test_data_parameter_jpeg.sh "$case_nr"

    if [ ${valid[${set_nro}]} -eq 0 ]
    then

        sh checkcase_jpeg.sh $case_nr
        res=$?
        extra_comment=${comment}" `cat case_${case_nr}/check.log`"

        getExecutionTime "$case_nr"

        if [ $CSV_REPORT -eq 1 ]
        then
            echo -n "case_$case_nr;Integration;JPEGCase;;$language;$timeform1;$timeform2;$execution_time;" >> $csvfile.csv
            echo -n "case_$case_nr;" >> $resultcsv
            case $res in
            0)
                echo "OK;$hwtag;$systag;$swtag;$reporter;$comment;;$kernelversion;$rootfsversion;$compilerversion;;" >> $csvfile.csv
                echo "OK;" >> $resultcsv
                let "sum_ok++"
                ;;
            1)
                echo "NOT_RUN;$hwtag;$systag;$swtag;$reporter;$comment;;$kernelversion;$rootfsversion;$compilerversion;;" >> $csvfile.csv
                echo "NOT_RUN;" >> $resultcsv
                let "sum_notrun++"
                ;;
            *)
                echo "FAILED;$hwtag;$systag;$swtag;$reporter;$comment;;$kernelversion;$rootfsversion;$compilerversion;;" >> $csvfile.csv
                echo "FAILED;" >> $resultcsv
                let "sum_failed++"
                ;;
            esac
        else
            cat case_${case_nr}/check.log
        fi
    fi
done

if [ $CSV_REPORT -eq 1 ]
then
    echo ""
    echo "Totals: $sum_ok OK   $sum_notrun NOT_RUN   $sum_failed FAILED"
    echo "Copying report $csvfile.csv to projects/8290/integration/test_reports/"
    cp $csvfile.csv /afs/hantro.com/projects/8290/integration/test_reports/
fi
