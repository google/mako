module github.com/google/mako-examples

go 1.12

replace github.com/google/mako v0.0.0 => /tmp/copybara-out

require (
	github.com/golang/protobuf v1.3.2
	github.com/google/mako v0.0.0
	google.golang.org/grpc v1.22.1 // indirect
)
