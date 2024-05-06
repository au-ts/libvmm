# Usage should be ./bench.sh <BOARD> <TEST> OPTIONS
MICROKIT_SDK_PATH=~/ts-home/microkit/release/microkit-sdk-1.2.6


# Check to see if minimal options available
if [ "$#" -lt 2 ]; then
    echo "Usage should be ./bench.sh <BOARD> <TEST> OPTIONS"
    exit 1
fi

BOARD=$1
TEST=$2

SIZE_PATCH=1
RECOMPILE=1
BENCHMARK=0

END_STR="asdasd"
MQ_BOARD=""

while [ "$#" -gt 2 ]; do
    case $3 in
        --no-size-patch)
            SIZE_PATCH=0
            ;;
        --no-recompile)
            RECOMPILE=0
            ;;
        --benchmark)
            BENCHMARK=1
            ;;
        *)
            echo "Unknown option $3"
            exit 1
            ;;
    esac
    shift
done


COMPILE_COMMAND="make MICROKIT_SDK=$MICROKIT_SDK_PATH BOARD=$BOARD TEST=$TEST CONFIG=benchmark"

if [ $SIZE_PATCH -eq 1 ]; then
    COMPILE_COMMAND="$COMPILE_COMMAND WTF_PATCH=yes"
fi

if [ $RECOMPILE -eq 1 ]; then
    COMPILE_COMMAND="$COMPILE_COMMAND -B"
fi

if [ $BENCHMARK -eq 1 ]; then
    COMPILE_COMMAND="$COMPILE_COMMAND BENCH=vmm"
    END_STR="END_RESULTS"
fi

if [ "$TEST" == "no-op" ]; then
    COMPILE_COMMAND="$COMPILE_COMMAND SPECIAL=noop"
fi

if [ "$BOARD" == "odroidc4" ]; then
    MQ_BOARD="odroidc4_pool"
elif [ "$BOARD" == "maaxboard" ]; then
    MQ_BOARD="maaxboard1"
else
    echo "Unknown board $BOARD"
    exit 1
fi

LOGFILE_NAME="logfile_$BOARD"
LOGFILE_NAME+="_$TEST"

echo "Compiling with command: $COMPILE_COMMAND"
$COMPILE_COMMAND


MQ_COMMAND="mq run -f output/$BOARD/$TEST/loader.img -s $MQ_BOARD -c $END_STR -l $LOGFILE_NAME"
echo "Running with command: $MQ_COMMAND"
$MQ_COMMAND

