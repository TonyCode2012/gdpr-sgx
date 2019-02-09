export const linksRegistry = [{
    name: "home",
    path: "/",
    label: "Home"
  }, {
    name: "register",
    path: "/register",
    label: "Sign Up"
  },
  {
    name: "login",
    path: "/login",
    label: "Sign On"
  },
  {
    name: "personal",
    path: "/demo/personal",
    label: "Personal User Entry"
  },
  {
    name: "business",
    path: "/demo/business",
    label: "Business User Entry"
  }
];

export function findLinkConfig(name) {
  return linksRegistry.find(config => config.name === name) || {};
}
