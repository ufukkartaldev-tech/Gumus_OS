# GümüşOS Geliştirme Yol Haritası (Uyanış) 🚀

Bu belge, GümüşOS'un gelişimini takip eder. Gümüşhane'den doğan bu güneş, adım adım dünyaya yayılıyor.

## 1. Temel Altyapı (Halledildi ✅)
- [x] **Bootloader:** 16-bit Real Mode'dan geçiş.
- [x] **Protected Mode:** 32-bit geçişi ve GDT kurulumu.
- [x] **IDT & Kesmeler:** Klavye ve kritik hata işleyicileri.
- [x] **Türkçe Karakter Desteği:** VGA Font yükleme ve UTF-8 (ğ, ş, ı...).

## 2. Bellek Sistemi 🧠
- [x] **PMM:** Bitmap tabanlı fiziksel bellek yönetimi (128MB RAM desteği).
- [x] **Heap Allocator:** `kmalloc` ve `kfree` (Linked List tabanlı, coalescing destekli).
- [x] **Paging:** 4KB sayfalarla sanal bellek yönetimi ve süreç izolasyonu. ✅
- [x] **Ring 3 (User Mode):** Kullanıcı modu geçişleri ve TSS stack yönetimi. ✅
- [x] **Syscalls:** `int 0x80` üzerinden sistem çağrıları (Write, Read vb.). ✅

## 3. Donanım Sürücüleri (Drivers) 🛠️
- [x] **Klavye:** TR-Q haritalamalı interrupt bazlı sürücü.
- [x] **Disk (ATA/IDE):** Sektör bazlı okuma/yazma (PIO Mode).
- [x] **Grafik:** VGA Mode 13h veya VBE (LFB) grafik sürücüsü. (Görselliğin zirvesi!)
- [x] **Ağ Sürücüleri:** Ethernet, ARP, IP, ICMP protokolleri.

## 4. Ağ Protokolleri 🌐
- [x] **Ethernet:** Temel frame gönderme/alma.
- [x] **ARP:** IP'den MAC çözümleme ve cache yönetimi.
- [x] **IPv4:** Paket yönetimi ve routing.
- [x] **ICMP:** Ping desteği.
- [ ] **UDP:** Datagram iletişimi.
- [ ] **TCP:** Güvenilir bağlantı yönetimi.

## 5. Dosya Sistemi & GümüşDil 💾
- [x] **Dosya Sistemi:** FAT12/16 prototipi (Listele, Oku, Kaydet).
- [x] **GümüşDil:** Bellek üzerinde koşan "soyle" interpreter'ı.
- [ ] **Dinamik Yükleyici:** Diskten program yükleyip çalıştırma.

## 6. Kullanıcı Arayüzü (GUI/TUI) 🎨
- [x] **Durum Çubuğu:** Sistem bilgilerini gösteren sabit bar.
- [x] **Pencere Sistemi:** Text mode üzerinde pencere çizimi.
- [x] **Renk Paleti:** Hata, başarı ve bilgi kodlu terminal.
- [x] **Grafiksel Dosya Yöneticisi:** Modern dosya yönetimi arayüzü.
- [x] **Sistem Monitörü:** Gerçek zamanlı performans izleme.

---
**Güncel Hedef:** UDP/TCP protokollerini tamamlamak veya NE2000 network kartı sürücüsünü geliştirmek.
