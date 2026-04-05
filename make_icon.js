// Yazıt – Orkhon kitabesi ikonu
// Referans: dikit taş, mavi gökyüzü, kazıma runik yazılar

const fs = require('fs');

// ─── Piksel tuval ────────────────────────────────────────────────────────────
function makeCanvas(size) {
    return Array.from({length: size}, () => new Uint32Array(size));
}
function px(canvas, size, x, y, argb) {
    if (x >= 0 && x < size && y >= 0 && y < size) canvas[y][x] = argb;
}
function blend(bg, fg, a) { // a: 0..255
    const br = (bg >> 16) & 0xFF, bg_ = (bg >> 8) & 0xFF, bb = bg & 0xFF;
    const fr = (fg >> 16) & 0xFF, fg_ = (fg >> 8) & 0xFF, fb = fg & 0xFF;
    const a1 = a / 255;
    return 0xFF000000 |
        (Math.round(br + (fr - br) * a1) << 16) |
        (Math.round(bg_ + (fg_ - bg_) * a1) << 8) |
        Math.round(bb + (fb - bb) * a1);
}

function createBitmap(size) {
    const cv = makeCanvas(size);
    const S = x => Math.round(x * size / 256); // ölçek (256 referans)

    // ─── Renkler ──────────────────────────────────────────────────────────────
    const SKY_TOP    = 0xFF0D2B5E; // koyu mavi gökyüzü
    const SKY_BTM    = 0xFF1A4F9C; // biraz daha açık
    const STONE_LITE = 0xFFCBC4B0; // taş açık (güneşli yüz)
    const STONE_MID  = 0xFFB0A892; // taş orta
    const STONE_DRK  = 0xFF8A8070; // taş koyu (kenar gölge)
    const CARVE_DRK  = 0xFF6B6255; // kazıma çukuru (karanlık)
    const CARVE_HI   = 0xFFDDD5C0; // kazıma kenar vurgu (ışık)
    const MORTAR     = 0xFF9A9282; // çizgi arası

    // ─── Gökyüzü gradyanı ─────────────────────────────────────────────────────
    for (let y = 0; y < size; y++) {
        const t = y / size;
        const r = Math.round(0x0D + (0x1A - 0x0D) * t);
        const g = Math.round(0x2B + (0x4F - 0x2B) * t);
        const b = Math.round(0x5E + (0x9C - 0x5E) * t);
        for (let x = 0; x < size; x++)
            cv[y][x] = 0xFF000000 | (r << 16) | (g << 8) | b;
    }

    // ─── Dikit (stele) şekli ──────────────────────────────────────────────────
    // Fotoğraftaki gibi: alta doğru geniş, üste doğru dar (perspektif)
    // Stele tabanı genişliği %70, tepesi %28 (merkezde)
    const steleBotW = S(180); // alt genişlik (yarı)
    const steleTopW = S(36);  // üst genişlik (yarı)
    const steleTop  = S(4);
    const steleBot  = size - S(2);
    const cx        = size / 2;

    function steleX(y) {
        const t = (y - steleTop) / (steleBot - steleTop);
        const halfW = steleTopW + (steleBotW - steleTopW) * t;
        return { left: cx - halfW, right: cx + halfW };
    }

    for (let y = steleTop; y < steleBot; y++) {
        const { left, right } = steleX(y);
        const w = right - left;

        for (let x = Math.floor(left); x <= Math.ceil(right); x++) {
            // Yatay gradyan: kenarlar koyu, orta açık (yuvarlak dikit)
            const rel = (x - left) / w; // 0..1
            // Silindirik ışık: ortası aydınlık, kenarlar gölgeli
            const light = Math.sin(rel * Math.PI); // 0→1→0
            const r = Math.round(0x8A + (0xCB - 0x8A) * light);
            const g = Math.round(0x80 + (0xC4 - 0x80) * light);
            const b = Math.round(0x70 + (0xB0 - 0x70) * light);
            px(cv, size, x, y, 0xFF000000 | (r << 16) | (g << 8) | b);
        }

        // Kenar gölgesi (ince şerit)
        for (let e = 0; e < S(3); e++) {
            const alpha = Math.round(180 * (1 - e / S(3)));
            const lx = Math.floor(left) + e;
            const rx = Math.ceil(right) - e;
            if (lx >= 0 && lx < size) {
                const old = cv[y][lx];
                cv[y][lx] = blend(old, 0x000000, alpha);
            }
            if (rx >= 0 && rx < size) {
                const old2 = cv[y][rx];
                cv[y][rx] = blend(old2, 0x000000, alpha);
            }
        }
    }

    // ─── Dikey çizgiler (sütun çizgileri, fotoğraftaki gibi) ─────────────────
    const numLines = 5;
    for (let i = 1; i < numLines; i++) {
        for (let y = steleTop + S(4); y < steleBot - S(2); y++) {
            const { left, right } = steleX(y);
            const lx = Math.round(left + (right - left) * i / numLines);
            // İnce dikey oluk
            for (let dy = -1; dy <= 0; dy++) {
                const c = cv[y + dy] ? cv[y + dy][lx] : 0;
                if (c) cv[y + dy][lx] = blend(c, 0x000000, 60);
            }
        }
    }

    // ─── Orkhon runik karakterler ──────────────────────────────────────────────
    // Karakterler dikey sütunlarda, kazıma efektiyle
    // Her karakter: koyu çukur + üst kenar aydınlık

    function carve(x, y, w, h) {
        if (isNaN(x) || isNaN(y) || isNaN(w) || isNaN(h)) return;
        x = Math.round(x); y = Math.round(y);
        w = Math.max(1, Math.round(w)); h = Math.max(1, Math.round(h));
        for (let dy = 0; dy < h; dy++) {
            for (let dx = 0; dx < w; dx++) {
                const cx2 = x + dx, cy2 = y + dy;
                if (cx2 < 0 || cx2 >= size || cy2 < 0 || cy2 >= size) continue;
                if (!cv[cy2]) continue;
                cv[cy2][cx2] = blend(cv[cy2][cx2], 0x000000, 110);
            }
        }
        // Üst kenar vurgu
        for (let dx = 0; dx < w; dx++) {
            const cx2 = x + dx, cy2 = y;
            if (cx2 >= 0 && cx2 < size && cy2 >= 0 && cy2 < size && cv[cy2]) {
                cv[cy2][cx2] = blend(cv[cy2][cx2], 0xFFFFFF, 60);
            }
        }
    }

    function carveLine(x1, y1, x2, y2, thickness) {
        const dx = x2 - x1, dy = y2 - y1;
        const len = Math.sqrt(dx * dx + dy * dy);
        const steps = Math.ceil(len);
        for (let i = 0; i <= steps; i++) {
            const fx = Math.round(x1 + dx * i / steps);
            const fy = Math.round(y1 + dy * i / steps);
            carve(fx - Math.floor(thickness / 2), fy - Math.floor(thickness / 2),
                  thickness, thickness);
        }
    }

    const t = Math.max(1, S(3));  // kazıma çizgi kalınlığı

    // Karakterleri 3 sütuna yerleştir
    // Sütun pozisyonları (stele içi, yatayda)
    const colPositions = [0.30, 0.50, 0.70];
    // Her sütunda 4 karakter, dikey

    const charHeight  = S(38);
    const charSpacing = S(8);
    const startY      = S(14);

    // Orkhon karakterleri: koordinatlar [0..1] x [0..1] içinde, normalize
    // Her karakter: çizgi listesi [[x1,y1,x2,y2], ...]
    const orkhonChars = [
        // 𐰔 Z/S harfi: dikey + iki çapraz dal
        [
            [0.5, 0.0, 0.5, 1.0],         // dikey gövde
            [0.5, 0.2, 1.0, 0.0],          // sağ üst dal
            [0.5, 0.8, 1.0, 1.0],          // sağ alt dal
        ],
        // 𐰖 Y harfi: V şekli + dikey
        [
            [0.5, 0.0, 0.5, 1.0],
            [0.5, 0.3, 0.0, 0.0],
            [0.5, 0.3, 1.0, 0.0],
        ],
        // 𐰣 N harfi: dikey + sol çapraz
        [
            [0.5, 0.0, 0.5, 1.0],
            [0.5, 0.25, 0.0, 0.1],
            [0.5, 0.75, 0.0, 0.9],
        ],
        // 𐱃 T harfi: dikey + yatay çıkıntılar
        [
            [0.5, 0.0, 0.5, 1.0],
            [0.3, 0.3, 0.7, 0.3],
            [0.3, 0.7, 0.7, 0.7],
        ],
        // 𐰴 Q harfi: dikey + sağ dallar
        [
            [0.5, 0.0, 0.5, 1.0],
            [0.5, 0.2, 0.9, 0.1],
            [0.5, 0.5, 0.9, 0.4],
            [0.5, 0.8, 0.9, 0.9],
        ],
        // 𐰭 NG harfi: iki dikey + yatay bağlantı
        [
            [0.25, 0.0, 0.25, 1.0],
            [0.75, 0.0, 0.75, 1.0],
            [0.25, 0.5, 0.75, 0.5],
        ],
        // 𐰽 S harfi: çapraz çizgiler
        [
            [0.1, 0.0, 0.9, 0.5],
            [0.1, 0.5, 0.9, 1.0],
            [0.5, 0.0, 0.5, 1.0],
        ],
        // 𐰀 A harfi: V ters
        [
            [0.5, 0.0, 0.1, 1.0],
            [0.5, 0.0, 0.9, 1.0],
            [0.2, 0.6, 0.8, 0.6],
        ],
        // 𐰉 B harfi: dikey + sağ kavis (kırık çizgi)
        [
            [0.4, 0.0, 0.4, 1.0],
            [0.4, 0.0, 0.9, 0.2],
            [0.4, 0.5, 0.9, 0.3],
            [0.4, 1.0, 0.9, 0.8],
        ],
        // 𐰞 L harfi: dikey + kısa sol çıkıntı ortada
        [
            [0.5, 0.0, 0.5, 1.0],
            [0.5, 0.45, 0.1, 0.35],
        ],
        // 𐰺 R harfi: diyagonal
        [
            [0.2, 0.0, 0.8, 1.0],
            [0.5, 0.0, 0.5, 0.5],
        ],
        // 𐰏 G harfi: köşeli U
        [
            [0.2, 0.0, 0.2, 0.8],
            [0.8, 0.0, 0.8, 0.8],
            [0.2, 0.8, 0.8, 0.8],
        ],
    ];

    const charIdx = [
        [0,  3, 6,  9],  // sol sütun
        [1,  4, 7, 10],  // orta sütun
        [2,  5, 8, 11],  // sağ sütun
    ];

    for (let col = 0; col < 3; col++) {
        const colRel = colPositions[col];

        for (let row = 0; row < 4; row++) {
            const ci = charIdx[col][row];
            const char = orkhonChars[ci % orkhonChars.length];

            const baseY   = startY + row * (charHeight + charSpacing);
            const charMidY = steleTop + baseY + charHeight / 2;

            // Stele sınırlarını kontrol et
            if (charMidY < steleTop || charMidY >= steleBot) continue;

            const { left: lx, right: rx } = steleX(Math.round(charMidY));
            const charCX = lx + (rx - lx) * colRel;
            const charW  = Math.max(S(18), (rx - lx) * 0.20);

            for (const [nx1, ny1, nx2, ny2] of char) {
                const ax = Math.round(charCX - charW / 2 + nx1 * charW);
                const ay = Math.round(steleTop + baseY + ny1 * charHeight);
                const bx = Math.round(charCX - charW / 2 + nx2 * charW);
                const by = Math.round(steleTop + baseY + ny2 * charHeight);
                if (!isNaN(ax) && !isNaN(ay) && !isNaN(bx) && !isNaN(by))
                    carveLine(ax, ay, bx, by, t);
            }
        }
    }

    // ─── Stele tepesinde hafif ışık parıltısı ─────────────────────────────────
    for (let y = steleTop; y < steleTop + S(12); y++) {
        const { left, right } = steleX(y);
        const fade = 1 - (y - steleTop) / S(12);
        for (let x = Math.floor(left); x <= Math.ceil(right); x++) {
            const old = cv[y] ? cv[y][x] : 0;
            if (old) cv[y][x] = blend(old, 0xFFFFFF, Math.round(80 * fade));
        }
    }

    return cv;
}

// ─── BMP/ICO dönüşüm ─────────────────────────────────────────────────────────
function makeDIB(canvas, size) {
    const buf = Buffer.alloc(size * size * 4);
    for (let y = 0; y < size; y++) {
        const row = canvas[size - 1 - y];
        for (let x = 0; x < size; x++) {
            const c   = row[x] || 0xFF000000;
            const off = (y * size + x) * 4;
            buf[off]     = (c >> 0)  & 0xFF; // B
            buf[off + 1] = (c >> 8)  & 0xFF; // G
            buf[off + 2] = (c >> 16) & 0xFF; // R
            buf[off + 3] = (c >> 24) & 0xFF; // A
        }
    }
    return buf;
}

function makeBMPHeader(size, dataLen) {
    const h = Buffer.alloc(40);
    h.writeUInt32LE(40, 0);
    h.writeInt32LE(size, 4);
    h.writeInt32LE(size * 2, 8);
    h.writeUInt16LE(1, 12);
    h.writeUInt16LE(32, 14);
    h.writeUInt32LE(0, 16);
    h.writeUInt32LE(dataLen, 20);
    return h;
}

function buildICO(sizes) {
    const entries = sizes.map(sz => {
        const pixels = makeDIB(createBitmap(sz), sz);
        const header = makeBMPHeader(sz, pixels.length);
        return { sz, header, pixels };
    });

    const dirOffset = 6 + entries.length * 16;
    let dataOffset  = dirOffset;

    const hdr = Buffer.alloc(6);
    hdr.writeUInt16LE(0, 0);
    hdr.writeUInt16LE(1, 2);
    hdr.writeUInt16LE(entries.length, 4);

    const dirs = entries.map(e => {
        const dataSize = e.header.length + e.pixels.length;
        const d = Buffer.alloc(16);
        d[0] = e.sz >= 256 ? 0 : e.sz;
        d[1] = e.sz >= 256 ? 0 : e.sz;
        d.writeUInt16LE(1, 4);
        d.writeUInt16LE(32, 6);
        d.writeUInt32LE(dataSize, 8);
        d.writeUInt32LE(dataOffset, 12);
        dataOffset += dataSize;
        return d;
    });

    const parts = [hdr, ...dirs];
    for (const e of entries) { parts.push(e.header); parts.push(e.pixels); }
    return Buffer.concat(parts);
}

const ico = buildICO([16, 32, 48, 64, 128, 256]);
fs.writeFileSync('D:/Uzay/src/app.ico', ico);
console.log('İkon oluşturuldu:', ico.length, 'byte');
