# Embench Benchmarks - Saral Bhasha Mein

## Kya hai ye Embench?

Embench ek test kit hai jo chote devices (jaise smart watch, IoT sensors) ki performance check karta hai. Isme 19 alag-alag programs hain jo real-life kaam karte hain.

---

## 19 Benchmarks - Ek Ek Karke Samajhte Hain

### 1. **aha-mont64**
**Kya karta hai**: Bade numbers ka multiplication karta hai special tarike se

**Simple explanation**: 
Jaise aap calculator pe 2 bade numbers multiply karte ho, waise hi ye program bhi karta hai. Lekin ye **modular arithmetic** use karta hai jo **encryption** (password lock) mein kaam aata hai. Jaise WhatsApp mein aapka message lock hota hai, waise hi.

**Example**: RSA encryption (bank apps mein use hota hai)

---

### 2. **crc32**
**Kya karta hai**: Data check karta hai ki kuch missing to nahi

**Simple explanation**: 
Jab aap koi file download karte ho, to ye check karta hai ki puri file aayi ya beech mein kuch chhut gaya. Jaise **checksum** - ek signature jo confirm kare ki data sahi hai.

**Example**: 
- File download complete hai ya nahi
- USB mein copy karte time data corrupt to nahi hua
- Network pe data bhejte time error detection

---

### 3. **cubic**
**Kya karta hai**: Equation solve karta hai (x³ + ax² + bx + c = 0)

**Simple explanation**: 
School mein aapne quadratic equation (x² wala) solve kiya tha na? Ye usse bhi complex cubic equation solve karta hai. **Control systems** mein use hota hai.

**Example**: 
- Robot ka path calculate karna
- Temperature control system mein calculation

---

### 4. **edn**
**Kya karta hai**: Structured data ko read aur process karta hai

**Simple explanation**: 
Jaise aap JSON ya XML file read karte ho, waise hi. Matlab **data ko samajhna aur organize karna**. IoT devices ko data bhejne-lene mein kaam aata hai.

**Example**: 
- Weather sensor ka data parse karna
- Smart home devices ka configuration read karna

---

### 5. **huffbench**
**Kya karta hai**: Data ko compress aur decompress karta hai

**Simple explanation**: 
Jaise ZIP file banate ho to file chhoti ho jati hai, waise hi. Ye **Huffman coding** algorithm use karta hai jo data ko kam jagah mein store karta hai.

**Example**: 
- Image compress karna
- File size kam karna storage bachane ke liye
- Data transmission mein bandwidth bachana

---

### 6. **matmult-int**
**Kya karta hai**: Do matrices ka multiplication karta hai

**Simple explanation**: 
Math mein aapne matrix seekha hoga. Ye do matrices ko multiply karta hai. **Image processing** aur **graphics** mein bahut use hota hai.

**Example**: 
```
[1 2]   [5 6]   [19 22]
[3 4] × [7 8] = [43 50]
```
- Image filters apply karna
- 3D graphics calculations
- Machine learning mein

---

### 7. **minver**
**Kya karta hai**: Matrix ka inverse nikalti hai

**Simple explanation**: 
Jaise division ka opposite multiplication hai, waise matrix ka inverse. Ye **linear equations** solve karne mein kaam aata hai. Robotics aur control systems mein bahut useful.

**Example**: 
- Robot arm ki position calculate karna
- Signal processing
- Navigation systems

---

### 8. **nbody**
**Kya karta hai**: Kai objects ke beech gravity simulation karta hai

**Simple explanation**: 
Jaise planets ek dusre ko attract karte hain gravity se, waise simulate karta hai. Ye **physics simulation** hai jo **collision detection** mein bhi use hota hai.

**Example**: 
- Game development mein physics
- Robot path planning
- Drone navigation

---

### 9. **nettle-aes**
**Kya karta hai**: Data ko encrypt karta hai (lock karta hai)

**Simple explanation**: 
Ye **AES encryption** hai - duniya mein sabse zyada use hone wala encryption. Jaise aap password se file lock karte ho. Banking apps, WhatsApp sab isme use karte hain.

**Example**: 
- Password protect files
- Secure communication (HTTPS)
- Banking transactions

---

### 10. **nettle-sha256**
**Kya karta hai**: Data ka unique fingerprint banata hai

**Simple explanation**: 
Jaise har insaan ki fingerprint alag hoti hai, waise har data ka bhi unique **hash** banata hai. Ek bhi letter change karo to pura hash change ho jata hai. Password storage mein use hota hai.

**Example**: 
- Password ko database mein safely store karna
- File integrity check
- Blockchain/cryptocurrency

---

### 11. **nsichneu**
**Kya karta hai**: Neural network (AI) simulation

**Simple explanation**: 
Ye chota **artificial brain** simulate karta hai. Jaise aapka dimaag patterns seekhta hai, waise hi. **Machine learning** ka basic version hai jo edge devices pe chalta hai.

**Example**: 
- Voice recognition (Alexa, Google Assistant)
- Image recognition (face unlock)
- Predictive text

---

### 12. **picojpeg**
**Kya karta hai**: JPEG image ko decode/open karta hai

**Simple explanation**: 
Jab aap photo gallery mein image dekhte ho, to pehle **decompress** karni padti hai. Ye wohi kaam karta hai - compressed JPEG ko readable format mein convert karta hai.

**Example**: 
- Camera se photo dekhna
- Web browser mein images load karna
- Digital photo frames

---

### 13. **qrduino**
**Kya karta hai**: QR code generate karta hai

**Simple explanation**: 
Aapne wo square barcode dekhe honge na jo mobile se scan karte hain? Ye wahi **QR codes** banata hai. Data ko 2D pattern mein encode karta hai.

**Example**: 
- Payment QR codes (Paytm, PhonePe)
- WiFi password sharing
- Product information

---

### 14. **sglib-combined**
**Kya karta hai**: Data structures ke operations (list, sort, search)

**Simple explanation**: 
Ye basic programming operations karta hai jaise **list banana, sorting, searching**. Har program mein ye kaam karne padte hain - data ko organize karna aur dhoondhna.

**Example**: 
- Phone book mein naam sort karna
- List mein element search karna
- Data ko arrange karna

---

### 15. **slre**
**Kya karta hai**: Text pattern matching (Regular Expression)

**Simple explanation**: 
Jaise aap Microsoft Word mein "Find" karte ho, lekin isse bhi powerful. **Pattern matching** karta hai - jaise "saare email addresses dhundho" ya "phone numbers nikalo".

**Example**: 
- Email validation (@ aur .com check karna)
- Phone number format check
- Text parsing aur extraction

---

### 16. **st**
**Kya karta hai**: Statistical calculations (average, standard deviation)

**Simple explanation**: 
Math class mein aapne **mean, median, standard deviation** seekha tha na? Ye wohi calculations karta hai sensor data pe. Data analysis ke liye.

**Example**: 
- Temperature sensor ka average nikalana
- Data quality check
- Trend analysis

---

### 17. **statemate**
**Kya karta hai**: State machine execute karta hai

**Simple explanation**: 
Jaise traffic light - red se yellow, yellow se green, phir wapas red. Ye **states** aur **transitions** handle karta hai. Har embedded system mein use hota hai.

**Example**: 
- Washing machine ke different modes
- ATM machine ka flow
- Game levels aur screens

---

### 18. **ud**
**Kya karta hai**: Unicode text processing

**Simple explanation**: 
Alag alag languages (Hindi, Chinese, Arabic) ko computer samajh sake, uske liye **Unicode** hota hai. Ye wohi handle karta hai - different languages ka text process karna.

**Example**: 
- Hindi text display karna
- Emoji support
- Multilingual applications

---

### 19. **wikisort**
**Kya karta hai**: Data ko sort karta hai (arrange karna)

**Simple explanation**: 
Jaise aap cards ko arrange karte ho (A se K tak), waise hi. Ye **optimized sorting algorithm** hai jo fast aur memory-efficient hai.

**Example**: 
- Contacts ko alphabetically arrange karna
- Files ko name se sort karna
- Leaderboard rankings

---

## Category-wise Summary

### 🔐 **Security/Encryption** (3 programs)
- **nettle-aes**: Data lock karna
- **nettle-sha256**: Password hash/fingerprint
- **aha-mont64**: Cryptography maths

### 📦 **Compression** (2 programs)
- **huffbench**: Data ko chhota karna
- **picojpeg**: Image decompress karna

### 🧮 **Maths/Calculations** (5 programs)
- **cubic**: Equations solve karna
- **matmult-int**: Matrix multiply
- **minver**: Matrix inverse
- **nbody**: Physics simulation
- **st**: Statistics calculation

### 📊 **Data Handling** (4 programs)
- **sglib-combined**: Lists, sorting, searching
- **wikisort**: Fast sorting
- **slre**: Pattern matching
- **edn**: Data parsing

### 🤖 **Special Applications** (3 programs)
- **qrduino**: QR code banana
- **statemate**: State machines
- **nsichneu**: AI/Neural network

### 🔧 **Basic Operations** (2 programs)
- **crc32**: Data integrity check
- **ud**: Unicode text handling

---

## Kyun Important Hain Ye Benchmarks?

1. **Real Applications**: Ye sab real-life mein use hone wale programs hain, nakli test nahi
2. **Performance Check**: Ye batate hain ki aapka microcontroller kitna fast hai
3. **Comparison**: Different processors compare kar sakte ho
4. **Optimization**: Compiler settings test kar sakte ho
5. **Small Size**: Sab programs chhote hain (64KB) - embedded devices ke liye perfect

---

## Kaise Kaam Karta Hai?

1. Har benchmark ko **4 seconds** chalaya jata hai
2. Kitne **cycles** (processor steps) lage, wo count karte hain
3. Sab benchmarks ka **geometric mean** nikaal te hain
4. Ek **score** milta hai jo performance batata hai

**Simple analogy**: Jaise school mein alag alag subjects ke marks ka average nikalte ho, waise hi sab benchmarks ka average performance score milta hai.

---

## Kahan Use Hota Hai?

- **IoT devices**: Smart sensors, wearables
- **Embedded systems**: Washing machine, AC, car electronics
- **Microcontrollers**: Arduino, Raspberry Pi Pico type devices
- **Compiler testing**: GCC, Clang optimization check
- **Hardware comparison**: ARM vs RISC-V vs x86

---

## Final Summary

**Embench = 19 mini programs** jo alag alag kaam karte hain:
- Kuch **security** ka kaam (encryption, hashing)
- Kuch **data processing** (compression, parsing)
- Kuch **maths** ka kaam (matrix, equations)
- Kuch **practical applications** (QR codes, sorting, AI)

Ye sab milke **overall performance** batate hain ki aapka embedded system kitna powerful hai!

---

*Umeed hai ab sab clear ho gaya! 😊*
