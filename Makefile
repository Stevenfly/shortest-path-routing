all:
	@echo "Compiling router..."
	g++ -std=c++14 -o router router.cpp -Wall

clean:
	rm router
