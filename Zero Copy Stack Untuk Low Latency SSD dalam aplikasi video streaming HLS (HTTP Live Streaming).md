# Zero Copy Stack Untuk Low Latency SSD dalam aplikasi video streaming HLS (HTTP Live Streaming)
## Problem : 
- Bervariasinya jumlah akses pervideo
- Penggunaan memori yang tinggi, pada saat memerlukan buffer untuk menyimpan data video yang sedang diproses.
- Latensi tinggi dalam pengolahan data video, dikarenakan adanya proses salinan data (copy) yang dilakukan.
## Solution :
- Menggunakan cache DRAM untuk video yang sering diakses, bila tidak ada di cache, maka data akan diambil dari SSD menggunakan zero copy.
## Referensi :
- https://sci-hub.se/https://ieeexplore.ieee.org/document/9373910
- http://files.andrewsblog.org/http_live_streaming.pdf
- https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=4696190
- https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=10758644