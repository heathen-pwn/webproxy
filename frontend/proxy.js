window.addEventListener('beforeunload', function(event) {
    // Custom message for the confirmation dialog
    // event.returnValue = 'Are you sure you want to leave this page?';
    event.preventDefault();
});
document.addEventListener("DOMContentLoaded", function () {
    // Prevent window.location.assign
    window.location.assign = function() {
        console.warn('Blocked attempt to change window.location using assign:', arguments);
    };

    // Prevent window.location.replace
    window.location.replace = function() {
        console.warn('Blocked attempt to change window.location using replace:', arguments);
    };

    // Prevent window.location.reload
    window.location.reload = function() {
        console.warn('Blocked attempt to reload the page');
    };

    // Prevent window.history.pushState
    const originalPushState = window.history.pushState;
    window.history.pushState = function(state, title, url) {
        console.warn('Blocked attempt to push a new state:', { state, title, url });
    };

    // Prevent window.history.replaceState
    const originalReplaceState = window.history.replaceState;
    window.history.replaceState = function(state, title, url) {
        console.warn('Blocked attempt to replace the current state:', { state, title, url });
    };

    // Prevent window.open
    window.open = function() {
        console.warn('Blocked attempt to open a new window:', arguments);
    };

    
    // Anchor links
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
