.PHONY: all driver app test demo clean

all: driver app

driver:
	@echo "=== Building Linux Character Device Driver ==="
	$(MAKE) -C driver

app:
	@echo "=== Building C++ Warehouse Management System ==="
	mkdir -p build && cd build && cmake .. && $(MAKE) -j$(shell nproc 2>/dev/null || echo 2)

test: app
	@echo "=== Running Comprehensive Test Suites ==="
	./build/tests/wms_tests

demo: app
	@echo "=== Running Live System Demonstration ==="
	./build/wms_app --demo

dashboard:
	@echo "=== Launching Interactive Web Dashboard ==="
	@bash scripts/open_dashboard.sh || true

clean:
	@echo "=== Cleaning Project Artifacts ==="
	$(MAKE) -C driver clean || true
	rm -rf build
