if [ "$#" -lt 1 ]; then
  echo "Usage: $0 file"
  exit 1
fi

SRC=$1

compile_start=$SECONDS
g++ -std=c++23 -O3 "$SRC" -o run_binary
compile_elapsed=$(( SECONDS - compile_start ))
echo "Compilation time: ${compile_elapsed}s"

echo "Execution start: $(date -Iseconds)"
run_start=$SECONDS
./run_binary
run_elapsed=$(( SECONDS - run_start ))
echo "Execution time: ${run_elapsed}s"
