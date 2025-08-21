# Aplikasi zero copy pada http request chunking
## Problem :
- Latensi tinggi dalam pengolahan data HTTP request chunking, dikarenakan adanya proses salinan data (copy) yang dilakukan.
- Penggunaan memori yang tinggi, pada saat memerlukan buffer untuk menyimpan data yang sedang diproses.
## Solution :
- Menggunakan zero copy untuk mengurangi latensi dan penggunaan memori, dengan cara menghindari proses salinan data yang tidak perlu.
## Referensi
- https://ieeexplore.ieee.org/stamp/stamp.jsp?tp=&arnumber=4696190
- https://www.youtube.com/watch?v=bsyWXrTP8tU

## Concerns :
- keamanan sanitasi dan pengecekan data yang masuk