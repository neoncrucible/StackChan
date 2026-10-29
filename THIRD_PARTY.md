# Provenance and design references

The hardware port, setup and streaming voice integration derive from M5Stack StackChan, commit `f901cc2969189f3f0e96af1442c0f96e77e1e2c1`. Preserve source-level SPDX and copyright notices; bundled driver files and registry dependencies retain their individual licences. Xiaozhi is pinned at `e77dedb13091` (the full commit is in `firmware/repos.json`) with the recorded board patch.

PHOSPHOR's terminal face and audio meter are original code. Newly written PHOSPHOR files marked MIT may be used under the following terms:

Copyright (c) 2026 PHOSPHOR contributors

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

This grant covers the original PHOSPHOR additions, not a relicensing of dependencies. M5Stack MIT-marked portions retain Copyright (c) 2026 M5Stack Technology CO LTD and the same MIT permission and warranty terms. Other components retain the licence texts included in the binary package.

The repair priorities were informed by public Dotty fw-v1.3.4/v1.3.5 reports and source at `BrettKinny/StackChan@c074907965b0f1e12dd59968094b61ecfdb7cdd7`: blocking servo reads, joining receive tasks, allocation failures and recoverable codec opens. PHOSPHOR implements these repairs against its own pinned stock dependencies. The network build-copy technique preserves the registry's original verified files; the underlying `78/esp-ml307` component is Apache-2.0.

Stack-chan 2.0.1, Stackchan-IDF, Fairy and Kai informed the comparison. No Fairy/Nous character assets, voice packages or unpublished firmware patches are included. See the research report for source links and the limits of the review.
