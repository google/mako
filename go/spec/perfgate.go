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

// Package mako provides spec interfaces for all Mako components.
//
// Some components also provide a "Default" implementation which can be embedded
// to implement optional functions.
//
// See go/mako for help and more information.
package mako

import "errors"

// ErrDone error will be returned when the Context which is passed
// is marked as Done() before the respective operation is finished.
var ErrDone = errors.New("CONTEXT_DONE")
