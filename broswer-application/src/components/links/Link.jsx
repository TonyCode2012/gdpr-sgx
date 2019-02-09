import React, { Component } from "react";
import PropTypes from "prop-types";
import { withRouter } from "react-router-dom";
import { findLinkConfig } from "./linksRegistry";

class Link extends Component {
  constructor(props) {
    super(props);

    this.config = findLinkConfig(this.props.dest);

    this.handleOnClick = this.handleOnClick.bind(this);
  }

  handleOnClick() {
    const { onClick } = this.props;
    if (onClick && typeof onClick === "function") {
      onClick();
    } else {
      this.props.history.push(this.config.path);
    }
  }

  render() {
    const { label, ...rest } = this.props;

    return (
      <a href={this.config.path} {...rest}>
        {label || this.config.label}
      </a>
    );
  }
}
Link.propTypes = {
  dest: PropTypes.string.isRequired
};

Link.defaultProps = {
  dest: "register"
};

export default withRouter(Link);
