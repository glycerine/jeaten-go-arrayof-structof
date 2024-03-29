// Copyright 2009 The Go Authors.  All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// +build dragonfly freebsd linux nacl netbsd

package net

import (
	"os"
	"syscall"
	"time"
)

// Set keep alive period.
func setKeepAlivePeriod(fd *netFD, d time.Duration) error {
	if err := fd.incref(); err != nil {
		return err
	}
	defer fd.decref()

	// The kernel expects seconds so round to next highest second.
	d += (time.Second - time.Nanosecond)
	secs := int(d.Seconds())

	err := os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd.sysfd, syscall.IPPROTO_TCP, syscall.TCP_KEEPINTVL, secs))
	if err != nil {
		return err
	}
	return os.NewSyscallError("setsockopt", syscall.SetsockoptInt(fd.sysfd, syscall.IPPROTO_TCP, syscall.TCP_KEEPIDLE, secs))
}
