let board = new Array(128);
for (var i = 0; i < 128; i++) {
    board[i] = new Array(128);
    for (var j = 0; j < 128; j++) {
        board[i][j] = new Array(3);
    }
}

var squareDim = 0;

function setup() {
    squareDim = .9*min(windowWidth, windowHeight);
    createCanvas(squareDim, squareDim);
}

function windowResized() {
    squareDim = .9*min(windowWidth, windowHeight);
    resizeCanvas(squareDim, squareDim);

    for (var i = 0; i < 128; i++) {
        for (var j = 0; j < 128; j++) {
            drawPixel(i,j,board[i][j][0], board[i][j][1], board[i][j][2]);
        }
    }
}

let ws = new WebSocket("ws://0.0.0.0:8000/ws");

ws.onmessage = function(ev) {
    console.log(ev);
    if (ev.data.length > 100) {
        drawWholeTable(ev.data);
    } else {
        drawPixelFromString(ev.data);
    }
}

function drawPixelFromString(s) {
    let x = parseInt("0x" + s.substring(0,2));
    let y = parseInt("0x" + s.substring(2,4));
    let r = parseInt("0x" + s.substring(4,6));
    let g = parseInt("0x" + s.substring(6,8));
    let b = parseInt("0x" + s.substring(8,10));
    drawPixel(x,y,r,g,b);
}

function drawPixel(x,y,r,g,b) {
    board[x][y] = [r,g,b];

    let sideSize = squareDim / 128;
    
    noStroke();
    fill(color(r,g,b));
    square(x * sideSize, y * sideSize, sideSize+1);
}

function drawWholeTable(s) {
    for (var i = 0; i < 128; i++) {
        for (var j = 0; j < 128; j++) {
            let r = parseInt("0x" + s.substring(6*(i*128+j), 6*(i*128+j)+2));
            let g = parseInt("0x" + s.substring(6*(i*128+j)+2, 6*(i*128+j)+4));
            let b = parseInt("0x" + s.substring(6*(i*128+j)+4, 6*(i*128+j)+6));
            drawPixel(i, j, r, g, b);
        }
    }
}
