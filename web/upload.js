document.getElementById('uploadForm').addEventListener('submit', function(e) {
    e.preventDefault();

    const fileInput = document.getElementById('fileInput');
    const file = fileInput.files[0];
    
    // 设置文件大小限制（例如 5 MB）
    const maxFileSize = 10 * 1024 * 1024; // 10 MB

    if (file) {
        if (file.size > maxFileSize) {
            alert(`文件大小不能超过 ${maxFileSize / (1024 * 1024)} MB`);
            return; // 阻止提交表单
        }

        const formData = new FormData();
        formData.append('file', file); // 直接添加文件对象
        fetch('/upload', {
            method: 'POST',
            body: formData
        })
        .then(response => response.text())
        .then(result => alert(result))
        .catch(error => console.error('Error:', error));
    }
});
