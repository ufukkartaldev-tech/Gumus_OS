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

### 7. Grafiksel Dosya Yöneticisi 📁
Modern dosya yönetimi arayüzü!
- **Grid View:** İkonlu dosya görüntüleme
- **Mouse Desteği:** Çift tıklama ile dosya açma
- **Dosya İşlemleri:** Kopyala, taşı, sil, yeniden adlandır
- **Klasör Navigasyonu:** Alt ve üst dizinler arasında geçiş
- **Context Menu:** Sağ tık menüsü
- **Dosya Önizleme:** Metin ve resim dosyaları için hızlı önizleme

### 8. Sistem Monitörü 📊
Gerçek zamanlı sistem performans izleme!
- **CPU Kullanımı:** Anlık CPU kullanım yüzdesi ve geçmiş grafiği
- **Bellek Kullanımı:** RAM kullanım istatistikleri ve grafikler
- **Sistem Bilgileri:** Uptime, process sayısı, interrupt sayısı
- **Sürücü İstatistikleri:** Disk, network, input kullanım verileri
- **Grafiksel Arayüz:** Bar grafikler ve çizgi grafikler
- **Gerçek Zamanlı Güncelleme:** Otomatik veri yenileme

### 9. Gelişmiş Ses Sistemi 🎵
Profesyonel ses yönetimi ve işleme!
- **Çok Kanallı Ses:** 8 kanala kadar aynı anda ses çalma
- **Ses Formatları:** WAV ve MIDI dosya formatı desteği
- **Ses Karıştırıcı:** Multiple ses kanalını aynı anda çalma
- **Ses Efektleri:** Echo, reverb, distortion gibi efektler
- **Ses Kayıt:** Microphone ile ses kaydetme
- **Ses Kontrolü:** Volume, pan, loop kontrolleri
- **Grafiksel Mixer:** Profesyonel ses arayüzü
- **Equalizer:** 10 band grafiksel equalizer
- **Visualizer:** Gerçek zamanlı ses görselleştirme

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
4. **Dosya Yöneticisi:** Shell'den `dosyalar` komutu ile grafiksel dosya yöneticisini açın.
5. **Sistem Monitörü:** Shell'den `monitor` komutu ile sistem performansını izleyin.
6. **Ses Sistemi:** Shell'den `ses_cal` komutu ile ses mixer'ı açın.

---

## 📅 Yol Haritası (Uyanış Devam Ediyor)
GümüşOS'un gelişimini `ROADMAP.md` dosyasından takip edebilirsiniz. Bir sonraki kritik hedefler:
- [ ] Disk tabanlı dinamik program yükleyici (ELF/Binary Loader).
- [ ] GümüşDil (`soyle`) entegrasyonu.
- [ ] Pencere tabanlı grafiksel kullanıcı arayüzü (GUI) genişletmesi.

---
*Gümüşhane'den doğan bu güneş, adım adım dünyaya yayılıyor. Uyanış başladı...* 🌌
