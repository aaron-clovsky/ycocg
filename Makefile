MAKEFLAGS+=--no-print-directory
CC=cc
CFLAGS=-std=c89 -Wpedantic -Wall -Wextra -O3 -march=native -mtune=native

TARGET_C_TOOL=ycocg_tool
TARGET_C_TEST_SIMD_RAND=test_simd_rand
TARGET_C_TEST_SIMD_FULL=test_simd_full
TARGET_V_DEC=tb_ycocg_to_rgb_v
TARGET_SV_DEC=tb_ycocg_to_rgb_sv

DIR_INC=include
DIR_SRC=src
DIR_BIN=bin
DIR_RTL=rtl
DIR_TB=tb
DIR_SCRIPTS=scripts
DIR_SCRIPTS_LOGS=scripts/logs
DIR_VERILATOR=$(DIR_BIN)/verilator
DIR_TEST_OUTPUT=$(DIR_BIN)/output

.SILENT:
.PHONY: all clean style lint testbench

all: $(TARGET_C_TOOL) $(TARGET_C_TEST_SIMD_RAND) $(TARGET_C_TEST_SIMD_FULL) \
     $(TARGET_V_DEC) $(TARGET_SV_DEC)

$(TARGET_C_TOOL):
	$(CC) $(CFLAGS) -I$(DIR_INC) $(DIR_SRC)/ycocg_tool.c \
          -o $(DIR_BIN)/$(TARGET_C_TOOL)
$(TARGET_C_TEST_SIMD_RAND):
	$(CC) $(CFLAGS) -I$(DIR_INC) $(DIR_SRC)/test_simd_rand.c \
          -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
$(TARGET_C_TEST_SIMD_FULL):
	$(CC) $(CFLAGS) -I$(DIR_INC) $(DIR_SRC)/test_simd_full.c \
          -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
$(TARGET_V_DEC):
	if command -v verilator >/dev/null 2>&1; then \
	  verilator --timing -Wall -j 0 --binary --Mdir $(DIR_VERILATOR) \
	            --quiet --top-module tb_ycocg_to_rgb -o ../$(TARGET_V_DEC) \
	            $(DIR_RTL)/ycocg_to_rgb.v $(DIR_TB)/tb_ycocg_to_rgb.v; \
	else \
	  echo "WARNING: Verilator is not installed; skipping Verilog testbench"; \
	fi
$(TARGET_SV_DEC):
	if command -v verilator >/dev/null 2>&1; then \
	verilator --timing -Wall -j 0 --binary --Mdir $(DIR_VERILATOR) \
	          --quiet --top-module tb_ycocg_to_rgb -o ../$(TARGET_SV_DEC) \
	          $(DIR_RTL)/ycocg_to_rgb.sv $(DIR_TB)/tb_ycocg_to_rgb.sv; \
	else \
	  echo "WARNING: Verilator is not installed;" \
           "skipping SystemVerilog testbench"; \
	fi

testbench:
	$(DIR_SCRIPTS)/testbench.sh

clean:
	rm -f $(DIR_BIN)/$(TARGET_C_TOOL)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	rm -f $(DIR_BIN)/$(TARGET_V_DEC)
	rm -f $(DIR_BIN)/$(TARGET_SV_DEC)
	rm -rf $(DIR_VERILATOR)
	rm -rf $(DIR_TEST_OUTPUT)
	rm -rf $(DIR_SCRIPTS_LOGS)

style:
	command -v clang-format >/dev/null 2>&1 || \
	         { echo "Error: clang-format is not installed."; exit 1; }
	VERSION=$$(clang-format --version | sed -E 's/.*version ([0-9]+).*/\1/'); \
	[ "$$VERSION" -ge "21" ] || \
	    { echo "Error: clang-format must be at least " \
	           "version 21 ($$VERSION installed)"; exit 1; }
	clang-format -i -style=file:clang_format \
	              $(DIR_INC)/*.h $(DIR_SRC)/*.c \
                  $(DIR_RTL)/*.v $(DIR_RTL)/*.sv $(DIR_TB)/*.v $(DIR_TB)/*.sv

lint:
	echo Linting...
	command -v gcc >/dev/null 2>&1 || \
	        { echo "Error: gcc is not installed."; exit 1; }
	
	echo " gcc in C mode: $(DIR_SRC)/ycocg_tool.c:"
	gcc -std=c89 -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	    $(DIR_SRC)/ycocg_tool.c -o $(DIR_BIN)/$(TARGET_C_TOOL)
	rm -f $(DIR_BIN)/$(TARGET_C_TOOL)
	
	echo " gcc in C mode: $(DIR_SRC)/test_simd_rand.c:"
	gcc -std=c89 -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	    $(DIR_SRC)/test_simd_rand.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	
	echo " gcc in C mode: $(DIR_SRC)/test_simd_full.c:"
	gcc -std=c89 -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	    $(DIR_SRC)/test_simd_full.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	
	command -v g++ >/dev/null 2>&1 || \
	        { echo "Error: g++ is not installed."; exit 1; }
	
	echo " gcc in C++ mode: $(DIR_SRC)/ycocg_tool.c:"
	g++ -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	    $(DIR_SRC)/ycocg_tool.c -o $(DIR_BIN)/$(TARGET_C_TOOL)
	rm -f $(DIR_BIN)/$(TARGET_C_TOOL)
	
	echo " gcc in C++ mode: $(DIR_SRC)/test_simd_rand.c:"
	g++ -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	    $(DIR_SRC)/test_simd_rand.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	
	echo " gcc in C++ mode: $(DIR_SRC)/test_simd_full.c:"
	g++ -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	    $(DIR_SRC)/test_simd_full.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	
	command -v clang >/dev/null 2>&1 || \
	        { echo "Error: clang is not installed."; exit 1; }
	
	echo " clang in C mode: $(DIR_SRC)/ycocg_tool.c:"
	clang -std=c89 -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	      $(DIR_SRC)/ycocg_tool.c -o $(DIR_BIN)/$(TARGET_C_TOOL)
	rm -f $(DIR_BIN)/$(TARGET_C_TOOL)
	
	echo " clang in C mode: $(DIR_SRC)/test_simd_rand.c:"
	clang -std=c89 -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	      $(DIR_SRC)/test_simd_rand.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	
	echo " clang in C mode: $(DIR_SRC)/test_simd_full.c:"
	clang -std=c89 -Wpedantic -Wall -Wextra -Werror -I$(DIR_INC) \
	      $(DIR_SRC)/test_simd_full.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	
	command -v clang++ >/dev/null 2>&1 || \
	        { echo "Error: clang++ is not installed."; exit 1; }
	
	echo " clang in C++ mode: $(DIR_SRC)/ycocg_tool.c:"
	clang++ -x c++ -Wpedantic -Wall -Wextra -I$(DIR_INC) \
	        $(DIR_SRC)/ycocg_tool.c -o $(DIR_BIN)/$(TARGET_C_TOOL)
	rm -f $(DIR_BIN)/$(TARGET_C_TOOL)
	
	echo " clang in C++ mode: $(DIR_SRC)/test_simd_rand.c:"
	clang++ -x c++ -Wpedantic -Wall -Wextra -I$(DIR_INC) \
	        $(DIR_SRC)/test_simd_rand.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_RAND)
	
	echo " clang in C++ mode: $(DIR_SRC)/test_simd_full.c:"
	clang++ -x c++ -Wpedantic -Wall -Wextra -I$(DIR_INC) \
	        $(DIR_SRC)/test_simd_full.c -o $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	rm -f $(DIR_BIN)/$(TARGET_C_TEST_SIMD_FULL)
	
	command -v cppcheck >/dev/null 2>&1 || \
	        { echo "Error: cppcheck is not installed."; exit 1; }
    
	echo " cppcheck $(DIR_INC)/ycocg.h: "
	cppcheck --enable=all --suppress=missingIncludeSystem \
	         --suppress=checkersReport --check-level=exhaustive \
	         --quiet --inconclusive --std=c89 --suppress=unusedFunction \
	         $(DIR_INC)/ycocg.h
    
	echo " cppcheck $(DIR_SRC)/ycocg_tool.c: "
	cppcheck --enable=all --suppress=missingIncludeSystem \
	         --suppress=checkersReport --check-level=exhaustive \
	         --quiet --inconclusive --std=c89 \
	         $(DIR_SRC)/ycocg_tool.c
	
	echo " cppcheck $(DIR_SRC)/test_simd_rand.c: "
	cppcheck --enable=all --suppress=missingIncludeSystem \
	         --suppress=checkersReport --check-level=exhaustive \
	         --quiet --inconclusive --std=c89 \
	         $(DIR_SRC)/test_simd_rand.c

	echo " cppcheck $(DIR_SRC)/test_simd_full.c: "
	cppcheck --enable=all --suppress=missingIncludeSystem \
	         --suppress=checkersReport --check-level=exhaustive \
	         --quiet --inconclusive --std=c89 \
	         $(DIR_SRC)/test_simd_full.c
