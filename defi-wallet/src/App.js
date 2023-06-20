/* eslint-disable react-hooks/exhaustive-deps */
/* eslint-disable eqeqeq */
import React, { useEffect, useState } from "react";
import Web3 from "web3";
import MyNFT from "./const/abi.json";

import { toast, ToastContainer } from "react-toastify";
import "react-toastify/dist/ReactToastify.css";

import "./App.css";
import axios from "axios";

let web3;
let contract;

const baseURI =
  "https://cloudbunny.mypinata.cloud/ipfs/QmYmjkbWYTYnmipgcwkJeUT4956XFXAb3onpNjgyjmiyPW/";

function App() {
  const netID = 0x4; //rinkeby

  const [items, setItems] = useState([]);
  const [walletAddress, setWalletAddress] = useState("");
  const [connectStatus, setConnectStatus] = useState(false);
  const [correctNet, setCorrectNet] = useState(false);

  const connectWallet = async () => {
    if (!window.ethereum) {
      toast.error("Please install Metamask!");
    } else {
      try {
        const accounts = await window.ethereum.request({
          method: "eth_requestAccounts",
        });
        const chainId = await window.ethereum.request({
          method: "eth_chainId",
        });
        setWalletAddress(accounts[0]);
        setConnectStatus(true);
        setCorrectNet(chainId == netID);

        toast.success("Wallet Connected Successfully!");

        window.ethereum.on("accountsChanged", (accounts) => {
          setWalletAddress(accounts[0]);
        });

        window.ethereum.on("chainChanged", async (chainId) => {
          setCorrectNet(chainId == netID);
        });
      } catch (e) {
        console.error(e);
        toast.error("User denied wallet connection!");
      }
    }
  };

  const switchNetwork = async () => {
    try {
      await window.ethereum.request({
        method: "wallet_switchEthereumChain",
        params: [{ chainId: "0x4" }],
      });
      setCorrectNet(true);
    } catch (e) {
      console.error(e);
      toast.error("User denied wallet connection!");
    }
  };

  const getURIs = async () => {
    web3 = new Web3(window.ethereum);
    contract = new web3.eth.Contract(MyNFT.abi, MyNFT.contractAddress);
    const ids = await contract.methods.walletOfOwner(walletAddress).call();
    var temp = [];
    for (var id of ids) {
      let image = "";
      let name = "";

      await axios
        .get(`${baseURI}${id}.json`)
        .then(function (res) {
          image = res.data.image;
          name = res.data.name;
        })
        .catch((err) => {
          console.log(err, "err");
        });

      const item = {
        id: id,
        image: image,
        name: name,
      };
      temp.push(item);
    }
    setItems(temp);
  };

  const updateState = async () => {
    if (correctNet) {
      if (walletAddress) {
        await getURIs();
      } else {
        setConnectStatus(false)
        setItems([])
      }
    } 
  };

  const mint = async () => {
    try {
      await contract.methods.mint(1).send({
        from: walletAddress,
      });
      toast.success("Success!");
      updateState();
    } catch (e) {
      console.log(e);
      toast.error("Failed to Mint!");
    }
  };

  useEffect(() => {
    updateState();
  }, [correctNet, walletAddress]);

  useEffect(() => {
    const onLoad = async () => {
      await connectWallet();
    };

    window.addEventListener("load", onLoad);
    return () => window.removeEventListener("load", onLoad);
  }, []);

  return (
    <div className="App">
      <div className={correctNet && connectStatus ? "authed-container" : "container"}>
        <p className="sub-text">View your NFT collection in the metaverse ✨</p>
        <div className="header-container">
          <div className="connected-container">
            {connectStatus && !correctNet && (
              <button
                className="cta-button submit-nft-button"
                onClick={switchNetwork}
              >
                Switch Network
              </button>
            )}

            {correctNet && connectStatus && (
              <button className="cta-button submit-nft-button" onClick={mint}>
                Mint
              </button>
            )}

            {!connectStatus && (
              <button
                className="cta-button submit-nft-button"
                onClick={connectWallet}
              >
                Connect Wallet
              </button>
            )}
          </div>
        </div>
        {correctNet && connectStatus && (
          <div className="nft-grid">
            {items.map((item, index) => (
              <div className="nft-item" key={index}>
                <img src={item.image} alt="" />
                <div>{item.name}</div>
              </div>
            ))}
          </div>
        )}
      </div>
      <ToastContainer
        position="top-right"
        autoClose={3000}
        hideProgressBar={false}
        newestOnTop={false}
        closeOnClick
        rtl={false}
        pauseOnFocusLoss
        draggable
        pauseOnHover
        theme="colored"
      />
    </div>
  );
}

export default App;
