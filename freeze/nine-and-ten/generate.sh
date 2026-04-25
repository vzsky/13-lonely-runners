g++ -std=c++23 -O3 main.cpp -o new_run
./new_run > new_nine
rm new_run

echo "done with new code"

curl -L -o nine-and-ten-lonely-runners-e17c415b8.zip https://github.com/t-tanupat/nine-and-ten-lonely-runners/archive/e17c415b807258806cd9192a17e9a51e20455a75.zip
mkdir -p nine-and-ten-source
tar -xzf nine-and-ten-lonely-runners-e17c415b8.zip \
  --strip-components=1 \
  -C nine-and-ten-source
rm -f nine-and-ten-lonely-runners-e17c415b8.zip

cd nine-and-ten-source
patch -p1 < ../collapser_patch.diff

chmod +x meta_lrc_nine.sh
./meta_lrc_nine.sh lrc_for_nine_runners.cpp 8 \
  47 53 59 61 67 71 73 79 83 89 97 101 103 107 109 \
  113 127 131 137 139 149 151 157 163 167 173 179 \
  181 191 193 197 199 211 223 227 229 233 239 241 results_nine.txt

mv results_nine.txt ../orig_nine
rm -rf nine-and-ten-source
