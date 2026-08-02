# Tutorial Instalasi FlatCAM

Tutorial ini menjelaskan cara menginstal dan menjalankan **FlatCAM** — fork
modern berbasis PyQt5 yang kompatibel dengan versi Python modern (diuji
dengan Python 3.12 di Windows). FlatCAM 8.5 klasik membutuhkan PyQt4 yang
sudah tidak didukung di Python modern, sehingga panduan ini menggunakan
kode berbasis PyQt5 yang terpelihara (FlatCAM beta 8.99x).

> Catatan: FlatCAM memiliki antarmuka GUI. Tidak ada "instalasi tanpa
> tampilan" — yang diinstal adalah kode sumber beserta dependensi Python-nya,
> lalu GUI dijalankan.

---

## 1. Prasyarat

Pastikan hal-hal berikut sudah terpasang di sistem Anda:

| Alat     | Versi               | Perintah cek         |
| -------- | ------------------- | -------------------- |
| Python   | 3.12 (atau 3.5+)    | `python --version`   |
| Git      | versi terbaru apa pun | `git --version`    |

Di Windows, Python menginstal `pip` secara otomatis. Verifikasi dengan:

```
pip --version
```

---

## 2. Ambil kode sumber

Klon fork FlatCAM yang siap untuk Python 3.12 ke dalam folder pilihan Anda
(di sini: `flatcam_modern`):

```bash
git clone --depth 1 https://codeberg.org/mrgkingcs/FlatCAM.git flatcam_modern
```

Flag `--depth 1` membuat klon dangkal (lebih cepat dan ringan).

---

## 3. Buat lingkungan virtual (venv)

Bekerja di dalam venv membuat dependensi FlatCAM terisolasi dari sistem Anda.

```bash
cd flatcam_modern
python -m venv .venv
```

Di Windows, aktifkan dengan:

```powershell
.\.venv\Scripts\activate
```

Di Linux/macOS:

```bash
source .venv/bin/activate
```

> Anda boleh melewati aktivasi dan langsung memanggil interpreter-nya,
> misalnya `.venv\Scripts\python.exe -m pip ...` di Windows.

Tingkatkan `pip` di dalam venv:

```bash
python -m pip install --upgrade pip
```

---

## 4. Instal dependensi

Instal semua paket yang tercantum di `requirements.txt`:

```bash
python -m pip install -r requirements.txt
```

Ini menginstal tumpukan inti: PyQt5, numpy, matplotlib, shapely, rtree,
ortools, rasterio, vispy, ezdxf, lxml, svglib, dan lainnya.

### Catatan GDAL (Windows)

`requirements.txt` juga mencantumkan `gdal`. Di Windows **tidak ada wheel
biner untuk Python 3.12**, sehingga `pip` mencoba mengompilasi dari sumber
dan gagal (tidak ada file `gdal.h`). Aman untuk dilewati:

- FlatCAM tidak pernah mengimpor `gdal` secara langsung — dukungan raster
  disediakan oleh `rasterio`, yang sudah menyertakan pustaka GDAL sendiri.
- Instal semua kecuali GDAL:

```powershell
Get-Content requirements.txt | Where-Object { $_ -notmatch '^\s*#|^gdal' } | Set-Content requirements_nogdal.txt
python -m pip install -r requirements_nogdal.txt
```

### pywin32 (khusus Windows)

Di Windows, FlatCAM membutuhkan modul `win32comext` untuk integrasi shell.
Instal secara terpisah:

```powershell
python -m pip install pywin32
```

Jika dilewati, FlatCAM akan crash saat startup dengan:
`ModuleNotFoundError: No module named 'win32comext'`.

---

## 5. Jalankan FlatCAM

Dari folder `flatcam_modern`:

```bash
python FlatCAM.py
```

Jendela GUI akan terbuka dan mencetak log debug ke konsol. Anda bisa
menghentikannya kapan saja dengan `Ctrl+C` di konsol (atau tutup jendelanya).

### Skrip peluncur untuk Windows

Untuk menjalankan dengan klik dua kali, letakkan ini di `Run_FlatCAM.bat`
di sebelah folder proyek (mengasumsikan `flatcam_modern` dan `.venv`
berada satu tingkat):

```bat
@echo off
cd /d "%~dp0flatcam_modern"
"%~dp0.venv\Scripts\python.exe" FlatCAM.py
pause
```

---

## 6. Verifikasi instalasi

Instalasi yang benar akan mencetak pesan inisialisasi seperti:

```
[DEBUG][MainThread] Finished creating Object Collection.
[DEBUG][MainThread] Finished Canvas initialization ...
[DEBUG][MainThread] Finished connecting Signals.
[DEBUG][MainThread] END of constructor. Releasing control.
```

Jika baris-baris tersebut muncul (atau GUI tampil), instalasi selesai.

---

## 7. Pemecahan masalah

| Masalah                                   | Penyebab                              | Perbaikan                                                  |
| ----------------------------------------- | ------------------------------------- | ---------------------------------------------------------- |
| `No module named 'win32comext'`           | pywin32 tidak terpasang (Windows)     | `python -m pip install pywin32`                            |
| Build GDAL gagal, `gdal.h` tidak ditemukan| Tidak ada wheel untuk Python 3.12 di Windows | Lewati `gdal`; `rasterio` menangani dukungan raster |
| Peringatan `invalid escape sequence`      | Tidak berbahaya, kode lama di Python 3.12 | Abaikan — tidak memengaruhi eksekusi                    |
| `ModuleNotFoundError: PyQt4`              | Salah kode (FlatCAM 8.5 lama)         | Gunakan fork PyQt5 dari langkah 2                          |
| GUI terbuka lalu langsung tertutup        | Dependensi hilang                      | Ulangi langkah 4 dan periksa traceback di konsol          |

---

## 8. Ringkasan struktur folder

Setelah menyelesaikan tutorial ini, folder proyek Anda terlihat seperti:

```
proyek_anda/
├── .venv/              # lingkungan virtual dengan semua dependensi
├── flatcam_modern/     # kode sumber FlatCAM
│   ├── FlatCAM.py      # titik masuk program
│   └── requirements.txt
└── Run_FlatCAM.bat     # (Windows) peluncur klik dua kali
```
