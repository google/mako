// Package main blah blah blah
package main

import (
	"fmt"
	"github.com/golang/protobuf/proto"
	"github.com/google/mako/helpers/go/quickstore"
	qpb "github.com/google/mako/helpers/proto/quickstore/quickstore_go_proto"
	"log"
)

func main() {
	fmt.Printf("Hello!\n")
	q, err := quickstore.NewAtAddress(&qpb.QuickstoreInput{BenchmarkKey: proto.String("ssss")}, "localhost:9813")
	if err != nil {
		fmt.Printf("Uh oh...\n")
		log.Fatalf("failed NewAtAddress: %v", err)
	}
	q.AddRunAggregate("mem", 123)
	out, err := q.Store()
	if err != nil {
		log.Println("q.Store error:", out.String(), err)
	}
	fmt.Printf("Done!\n")
}
