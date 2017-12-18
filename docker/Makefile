demo:
	cp ../src/Control/elevator-control ./ && strip ./elevator-control
	docker build . -f ./Dockerfile.demo -t elevator-control-demo:latest --rm

build:
	docker build . -f ./Dockerfile.builder -t elevator-control-demo:builder --rm

run:
	docker run -i -t elevator-control-demo /bin/bash
