# GümüşOS: Uyanış Algoritması 🌌

GümüşOS, bir bilgisayarın donanımdan yazılıma nasıl "uyandığını" keşfeden, Gümüşhane'den doğan minimal ama modern mimarili bir işletim sistemi çekirdeğidir. Bu proje, x86 mimarisinin derinliklerine inerek bellek koruma, çoklu görev ve grafiksel kullanıcı arayüzü temellerini öğretmeyi amaçlar.

---

## 🛠️ Teknik Mimari (Detaylı Analiz)

GümüşOS, basit bir çekirdekten modern bir "Zırhlı Çekirdek" (Hardened Kernel) yapısına evrilmiştir. İşte sistemin bel kemiğini oluşturan bileşenler:

### 1. VBE 2.0 Grafik ve LFB (Linear Frame Buffer) 🎨
Eski tip segmentli bellek modellerinden kurtulup, modern grafik kartlarının sunduğu yüksek performansa geçtik.
- **Çözünürlük:** 800x600 piksel, 256 renk.
- **LFB Teknolojisi:** Ekranın tamamı bellekte tek bir lineer dizi gibi maplenir. Bu sayede her piksele direkt erişim sağlanır.
- **Çift Tamponlama (Double Buffering):** Tüm çizimler önce arka plandaki bir RAM bloğunda yapılır, ardından tek bir işlemle ekrana basılır. Bu, arayüzdeki titremeleri (flicker) tamamen yok eder.

### 2. Sanal Bellek ve Paging (Bellek İzolasyonu) 🧠
Sistem artık her süreci birbirinden ve çekirdekten izole eder.
- **Dinamik Adresleme:** 4KB'lik sayfalar halinde bellek yönetilir. Bir uygulamanın sanal adresi (`virtual address`), donanım tarafından şeffaf bir şekilde gerçek RAM adresine (`physical address`) dönüştürülür.
- **Süreç Güvenliği:** Her işlemin (process) kendine ait bir Sayfa Dizini (`Page Directory`) vardır. Bir uygulama, komşusunun veya çekirdeğin belleğine izinsiz erişmeye çalıştığında işlemci seviyesinde durdurulur.

### 3. Kullanıcı Modu (Ring 3) ve Ring 0 Ayırımı 🛡️
"Kurşun geçirmez" bir sistem için işlemci yetki seviyeleri kullanılır.
- **Ring 0 (Kernel):** Tüm donanıma erişim yetkisi olan en üst seviye.
- **Ring 3 (User):** Uygulamaların kısıtlı ve güvenli alanda çalıştığı seviye.
- **TSS (Task State Segment):** Kullanıcı modundan çekirdek moduna geçiş sırasında yığınların (stack) güvenli bir şekilde değiştirilmesini yönetir.

### 4. Sistem Çağrıları (Syscalls - int 0x80) 🚪
Uygulamalar donanıma direkt erişemezler. İhtiyaç duyduklarında çekirdeğin "resmî kapısını" çalarlar.
- **Mekanizma:** Uygulama, `int 0x80` kesmesini tetikler. Çekirdek bu çağrıyı yakalar, isteği doğrular ve güvenli bir şekilde donanım işlemini gerçekleştirir.
- **İletişim:** `EAX` register'ı üzerinden işlem numarası (yaz, oku, kapat vb.) iletilir.

### 5. Çoklu Görev (Multitasking) ve Zamanlayıcı 🔄
Sistem, aynı anda birden fazla işi yapıyormuş illüzyonu yaratır.
- **Scheduler:** Round-Robin (Döner Sıra) algoritmasıyla her sürece adil işlemci zamanı verilir.
- **Preemptive:** Çekirdek, uygulamayı beklemeden `IRQ0` (Sistem saati) kesmesiyle kontrolü geri alabilir ve diğer göreve geçebilir.

### 6. Ağ Desteği 🌐
GümüşOS artık modern ağ protokollerini destekliyor!
- **Ethernet Sürücüsü:** NE2000 uyumlu network kartları için temel sürücü.
- **ARP Protokolü:** IP adreslerinden MAC adreslerini çözme.
- **IP Protokolü:** IPv4 paket yönetimi ve routing.
- **ICMP (Ping):** Ağ bağlantı testi için ping desteği.
- **Shell Komutları:** `ag_baslat`, `ag_durum`, `ping [IP]`, `arp [IP]`

---

## 📂 Proje Yapısı
- `src/boot/`: Bilgisayarın ilk saniyelerini yöneten 16-bit Assembly kodları (Real Mode -> Protected Mode).
- `src/kernel/core/`: Çekirdeğin ana motoru; bellek yönetimi, paging ve syscall handler'lar.
- `src/kernel/cpu/`: CPU seviyesindeki tablolar (GDT, IDT, TSS) ve multitasking.
- `src/kernel/drivers/`: Ekran kartı (VGA/VBE), klavye, mouse ve disk sürücüleri.
- `src/kernel/drivers/network/`: Ağ protokolleri (Ethernet, ARP, IP, ICMP).

---

## 🚀 Nasıl Çalıştırılır?

1. **Gerekli Araçlar:** NASM, GCC (MinGW), LD ve QEMU sisteminizde kurulu olmalıdır.
2. **Derleme:** PowerShell üzerinden `./build.ps1` komutunu verin.
3. **Sonuç:** QEMU üzerinde GümüşOS'un 800x600'lük renkli dünyası açılacaktır.

---

## 📅 Yol Haritası (Uyanış Devam Ediyor)
GümüşOS'un gelişimini `ROADMAP.md` dosyasından takip edebilirsiniz. Bir sonraki kritik hedefler:
- [ ] Disk tabanlı dinamik program yükleyici (ELF/Binary Loader).
- [ ] GümüşDil (`soyle`) entegrasyonu.
- [ ] Pencere tabanlı grafiksel kullanıcı arayüzü (GUI) genişletmesi.

---
*Gümüşhane'den doğan bu güneş, adım adım dünyaya yayılıyor. Uyanış başladı...* 🌌
