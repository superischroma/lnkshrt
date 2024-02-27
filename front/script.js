const logger_el = document.getElementById("status_log");

const RANDOM_ENDPOINT_CHARS = "qwertyuiopasdfghjkzxcvbnmQWERTYUOPASDFGHJKLZXCVBNM";

document.getElementById("submit").addEventListener("click" , e =>
{
    /** @type {HTMLInputElement} */
    let lts_el = document.getElementById("link_to_shorten");
    let link = lts_el.value.trim();
    /** @type {HTMLInputElement} */
    let endpoint_el = document.getElementById("endpoint_name");
    let endpoint = endpoint_el.value.trim();
    if (link === "")
        return logger_el.innerHTML = "enter a link";
    if (!URL.canParse(link))
        return logger_el.innerHTML = "enter a valid link";
    if (endpoint === "")
        endpoint = [0, 0, 0, 0, 0].map(() => RANDOM_ENDPOINT_CHARS[Math.floor(Math.random() * RANDOM_ENDPOINT_CHARS.length)]).join("");
    let url = new URL(link);
    fetch("/api/shorten", {
        method: "POST",
        body: new TextEncoder().encode(url.href + " " + endpoint).buffer
    })
    .then(resp =>
    {
        if (!resp.ok)
            logger_el.innerHTML = "requested failed";
        else
            logger_el.innerHTML = "available at /" + endpoint;
    })
    .catch(() => logger_el.innerHTML = "requested failed");
});