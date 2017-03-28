/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <fcntl.h>
#include <stddef.h>
#include <string.h>
#include <utils/Log.h>

#include <setjmp.h>
#include <signal.h>

jmp_buf restore_point;
struct sigaction sa;

void segfault_sigaction(int signal __unused, siginfo_t *si __unused, void *arg __unused)
{
	ALOGE("Segmentation fault! Ignoring.");
	longjmp(restore_point, SIGSEGV);
}

// shim NvOsMemcpy
void NvOsMemcpy(void *dest, const void *src, size_t size)
{
	int fault_code = setjmp(restore_point);
	if (fault_code == 0)
		memcpy(dest, src, size);
	else
		return;
}

__attribute__((constructor))
void libEvtLoading(void)
{
	// struct sigaction sa;

	memset(&sa, 0, sizeof(struct sigaction));
	sigemptyset(&sa.sa_mask);
	sa.sa_sigaction = segfault_sigaction;
	sa.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
}
