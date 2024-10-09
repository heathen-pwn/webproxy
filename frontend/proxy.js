document.addEventListener("DOMContentLoaded", function () {
    const links = document.querySelectorAll("a"); // Select all anchor elements

    links.forEach(link => {
        const currentDomain = window.location.hostname; // Get current domain
        const linkDomain = new URL(link.href).hostname; // Get domain from link's href

        if (currentDomain !== linkDomain) {
            link.href = "/proxy?q=" + link.href;
            console.log(link.href);
        }
    });
});
