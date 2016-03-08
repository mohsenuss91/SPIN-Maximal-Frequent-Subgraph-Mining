
all:	trips

trips:
	g++ trips.cpp buffer.cpp -O3 -o Trips

clean:
	rm -f Trips
