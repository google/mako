// Copyright 2019 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// see the license for the specific language governing permissions and
// limitations under the license.

/*
Package g3storage provides a google3 implementation of the mako
Storage interface.

This implementation is only for google3 use because it uses http over rpc
to communicate with the mako server (current GAE app).

For more information about mako see go/mako.

For more information about the interface see
https://github.com/google/mako/spec/go/storage.go

This package defers work to SWIG wrapping of the C++ Mako client.

NOTE: Because of SWIG, although a Context is passed into each of the functions
below it is ignored. For this implementation of Mako Storage, calls cannot
be cancelled via Context.

Because unit testing is trivial, an end-to-end test is used instead:
https://github.com/google/mako/internal/testing/e2e/google3_storage_go_test.go
*/
package g3storage

import (
	wrap "github.com/google/mako/clients/cxx/storage/go/g3storage_wrap"
	"github.com/google/mako/internal/go/wrappedstorage"
)

// Storage is a Mako storage client. Zero value is not usable. Use New() or NewWithHostname().
type Storage struct {
	*wrappedstorage.Simple
	wrapper wrap.Storage
}

// NewFromWrapper takes a raw Storage SWIG wrapper and returns a Storage.
func NewFromWrapper(w wrap.Storage) *Storage {
	return &Storage{wrappedstorage.NewSimple(w, func() { wrap.DeleteStorage(w) }), w}
}
