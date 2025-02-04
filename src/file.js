function loadScript(url, callback) {
    let script = document.createElement('script');
    script.type = 'text/javascript';
    script.src = url;

    script.onload = function () {
        console.log(url + ' loaded successfully');
        if (callback) callback();
    };

    script.onerror = function () {
        console.error('Error loading script: ' + url);
    };

    document.head.appendChild(script);
}

function sleep(ms) { return new Promise(resolve => setTimeout(resolve, ms)); }

let html2pdfUrl = 'https://cdnjs.cloudflare.com/ajax/libs/html2pdf.js/0.9.2/html2pdf.bundle.min.js';

loadScript(html2pdfUrl, async function () {
    let button = document.getElementById('next-page-button');
    let element = document.getElementById('scrolling-content');
    while (button && button.checkVisibility() === true) {
        button.click();
        await sleep(10000);
        let opt = {
            margin: 1,
            filename: 'generated.pdf',
            html2canvas: { scale: 2 },
            jsPDF: { unit: 'in', format: 'a4', orientation: 'portrait' }
        };
        html2pdf().set(opt).from(element).save().then(function () {
            console.log('PDF generated successfully');
        }).catch(function (error) { console.error('Error generating PDF:', error); });
    }
});